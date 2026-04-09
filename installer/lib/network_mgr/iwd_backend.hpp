#ifndef ULI_NETWORK_MGR_IWD_BACKEND_HPP
#define ULI_NETWORK_MGR_IWD_BACKEND_HPP

/*
 * IWD Backend (iwd_backend.hpp)
 * =============================
 * Communicates with the IWD (iNet Wireless Daemon) to scan and connect
 * to wireless networks.
 *
 * STRATEGY:
 *   - Detection, interface discovery, and scanning use NATIVE D-BUS
 *     via DBusPoint (talking to net.connman.iwd).
 *   - Connection with passphrase falls back to popen("iwctl --passphrase ...")
 *     because IWD requires registering a full Agent object on D-Bus to handle
 *     passphrase requests, which is too complex for a synchronous installer.
 *   - If D-Bus calls fail, ALL operations fall back to popen() CLI wrappers.
 */

#include <string>
#include <vector>
#include <memory>
#include <cstdio>
#include <array>
#include <algorithm>
#include "../dbus/dbuspoint.hpp"

namespace uli {
namespace network_mgr {

class IwdBackend {
public:
    struct CFileDeleter {
        void operator()(FILE* f) const { if (f) pclose(f); }
    };

    // ── Utility: run a shell command and capture stdout ──────────────
    static std::string exec_command(const char* cmd) {
        std::array<char, 256> buffer;
        std::string result;
        std::unique_ptr<FILE, CFileDeleter> pipe(popen(cmd, "r"));
        if (!pipe) return "";
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    /*
     * is_available()
     * ---------------
     * Checks if IWD is running by asking D-Bus if "net.connman.iwd" is
     * registered on the system bus. Falls back to `which iwctl`.
     */
    static bool is_available() {
        // Try D-Bus first: ask the bus daemon if iwd service is active
        if (uli::dbus::DBusPoint::is_service_active("net.connman.iwd")) {
            return true;
        }
        // Fallback: check if iwctl binary exists on PATH
        std::string res = exec_command("which iwctl 2>/dev/null");
        return !res.empty();
    }

    /*
     * get_wireless_interfaces()
     * --------------------------
     * Discovers wireless interface names (e.g., "wlan0") by walking the IWD
     * D-Bus object tree and finding objects that implement
     * net.connman.iwd.Device, then reading their "Name" property.
     *
     * D-Bus flow:
     *   1. find_objects_with_interface("net.connman.iwd", "/net/connman/iwd",
     *                                 "net.connman.iwd.Device")
     *      → returns e.g. ["/net/connman/iwd/0/4"]
     *
     *   2. For each device path, read the "Name" property:
     *      Properties.Get("net.connman.iwd.Device", "Name")
     *      → returns e.g. "wlan0"
     */
    static std::vector<std::string> get_wireless_interfaces() {
        std::vector<std::string> devices;

        // Try D-Bus native discovery
        auto device_paths = uli::dbus::DBusPoint::find_objects_with_interface(
            "net.connman.iwd", "/net/connman/iwd", "net.connman.iwd.Device");

        for (const auto& path : device_paths) {
            std::string name = uli::dbus::DBusPoint::get_string_property(
                "net.connman.iwd", path, "net.connman.iwd.Device", "Name");
            if (!name.empty()) {
                devices.push_back(name);
            }
        }

        // If D-Bus returned results, use them
        if (!devices.empty()) return devices;

        // Fallback: parse `iw dev` output
        std::string output = exec_command("iw dev 2>/dev/null | awk '$1==\"Interface\"{print $2}'");
        size_t pos = 0;
        std::string token;
        while ((pos = output.find('\n')) != std::string::npos) {
            token = output.substr(0, pos);
            if (!token.empty()) devices.push_back(token);
            output.erase(0, pos + 1);
        }
        return devices;
    }

    /*
     * scan_interface(iface)
     * ----------------------
     * Triggers a wireless scan on the given interface.
     *
     * D-Bus flow:
     *   1. Find the Station object path for this interface by:
     *      a. Finding all Device objects
     *      b. Matching the one whose Name == iface
     *      c. Checking if that same path also has net.connman.iwd.Station
     *         (IWD uses the same object path for Device and Station)
     *
     *   2. Call Station.Scan() on that path:
     *      destination:  "net.connman.iwd"
     *      path:         (discovered station path)
     *      interface:    "net.connman.iwd.Station"
     *      method:       "Scan"
     *
     *   Note: Scan() is an async operation on the daemon side. We call it
     *   and the caller should wait ~3 seconds before reading results.
     */
    static void scan_interface(const std::string& iface) {
        std::string station_path = find_station_path(iface);

        if (!station_path.empty()) {
            bool ok = uli::dbus::DBusPoint::call_method_void(
                "net.connman.iwd", station_path,
                "net.connman.iwd.Station", "Scan", 10000);
            if (ok) return;  // D-Bus scan succeeded
        }

        // Fallback to CLI
        std::string cmd = "iwctl station " + iface + " scan";
        exec_command(cmd.c_str());
    }

    /*
     * get_available_networks(iface)
     * ------------------------------
     * Returns a list of SSIDs visible on the given interface.
     *
     * D-Bus flow:
     *   1. Find the Station path for this interface (same as scan)
     *   2. Call GetOrderedNetworks() on net.connman.iwd.Station
     *      → Returns: ARRAY of STRUCT(OBJECT_PATH, INT16)
     *         Each entry is (network_object_path, signal_strength)
     *   3. For each network path, read the "Name" property:
     *      Properties.Get("net.connman.iwd.Network", "Name")
     *      → Returns the SSID string
     *
     * Fallback: parse `iwctl station <iface> get-networks` output.
     */
    static std::vector<std::string> get_available_networks(const std::string& iface) {
        std::vector<std::string> ssids;

        std::string station_path = find_station_path(iface);
        if (!station_path.empty()) {
            ssids = get_networks_via_dbus(station_path);
            if (!ssids.empty()) return ssids;
        }

        // Fallback: parse iwctl CLI output
        std::string cmd = "iwctl station " + iface + " get-networks | awk '{if (NR>4 && $1!=\"\") print $0}'";
        std::string out = exec_command(cmd.c_str());

        size_t pos = 0;
        while ((pos = out.find('\n')) != std::string::npos) {
            std::string line = out.substr(0, pos);
            if (!line.empty()) {
                auto cpos = line.find_first_not_of(" \e\t\r\n\x1B[0123456789;m*>");
                if (cpos != std::string::npos) {
                    std::string stripped = line.substr(cpos);
                    auto endpos = stripped.find(" psk ");
                    if (endpos == std::string::npos) endpos = stripped.find(" open ");
                    if (endpos == std::string::npos) endpos = stripped.find(" 8021x ");
                    if (endpos != std::string::npos) stripped = stripped.substr(0, endpos);
                    stripped.erase(stripped.find_last_not_of(" \t\r\n") + 1);
                    if (!stripped.empty()) {
                        if (std::find(ssids.begin(), ssids.end(), stripped) == ssids.end()) {
                            ssids.push_back(stripped);
                        }
                    }
                }
            }
            out.erase(0, pos + 1);
        }
        return ssids;
    }

    /*
     * connect_network(iface, ssid, password)
     * ----------------------------------------
     * Connects to a wireless network.
     *
     * NOTE: IWD requires an Agent object registered on D-Bus to handle
     * passphrase requests during Connect(). Implementing a full Agent
     * proxy is complex (requires async message loop + method dispatch),
     * so we delegate to iwctl's --passphrase flag via popen.
     *
     * For open networks (no password), we attempt D-Bus Connect() first.
     */
    static bool connect_network(const std::string& iface, const std::string& ssid, const std::string& password) {
        // Always use iwctl CLI for passphrase-based connections
        // (IWD Agent registration is too complex for sync installer)
        std::string cmd;
        if (password.empty() || password == "None") {
            cmd = "iwctl station " + iface + " connect \"" + ssid + "\"";
        } else {
            cmd = "iwctl --passphrase \"" + password + "\" station " + iface + " connect \"" + ssid + "\"";
        }
        std::string res = exec_command(cmd.c_str());
        return res.find("Operation failed") == std::string::npos;
    }

private:
    /*
     * find_station_path(iface)
     * -------------------------
     * Internal helper: discovers the D-Bus object path for the IWD Station
     * that corresponds to the given network interface name (e.g., "wlan0").
     *
     * Walk:
     *   /net/connman/iwd → children → for each, check Device.Name == iface
     *                                 AND has Station interface
     */
    static std::string find_station_path(const std::string& iface) {
        auto station_paths = uli::dbus::DBusPoint::find_objects_with_interface(
            "net.connman.iwd", "/net/connman/iwd", "net.connman.iwd.Station");

        for (const auto& path : station_paths) {
            // The Station and Device share the same object path in IWD
            std::string name = uli::dbus::DBusPoint::get_string_property(
                "net.connman.iwd", path, "net.connman.iwd.Device", "Name");
            if (name == iface) return path;
        }

        return "";
    }

    /*
     * get_networks_via_dbus(station_path)
     * -------------------------------------
     * Internal helper: calls GetOrderedNetworks on an IWD Station and
     * extracts SSID names from the returned network object paths.
     *
     * D-Bus reply format for GetOrderedNetworks():
     *   ARRAY of STRUCT { OBJECT_PATH network_path, INT16 signal_strength }
     *
     * For each network_path, we read Properties.Get("net.connman.iwd.Network", "Name")
     * to get the human-readable SSID.
     */
    static std::vector<std::string> get_networks_via_dbus(const std::string& station_path) {
        std::vector<std::string> ssids;

        DBusMessage* reply = uli::dbus::DBusPoint::call_method(
            "net.connman.iwd", station_path,
            "net.connman.iwd.Station", "GetOrderedNetworks", 10000);

        if (!reply) return ssids;

        DBusMessageIter iter;
        if (!dbus_message_iter_init(reply, &iter)) {
            dbus_message_unref(reply);
            return ssids;
        }

        // Top level is ARRAY
        if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
            dbus_message_unref(reply);
            return ssids;
        }

        DBusMessageIter array_iter;
        dbus_message_iter_recurse(&iter, &array_iter);

        while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_STRUCT) {
            DBusMessageIter struct_iter;
            dbus_message_iter_recurse(&array_iter, &struct_iter);

            // First element of struct: OBJECT_PATH
            if (dbus_message_iter_get_arg_type(&struct_iter) == DBUS_TYPE_OBJECT_PATH) {
                const char* net_path = nullptr;
                dbus_message_iter_get_basic(&struct_iter, &net_path);

                if (net_path) {
                    std::string ssid = uli::dbus::DBusPoint::get_string_property(
                        "net.connman.iwd", net_path,
                        "net.connman.iwd.Network", "Name");
                    if (!ssid.empty()) {
                        if (std::find(ssids.begin(), ssids.end(), ssid) == ssids.end()) {
                            ssids.push_back(ssid);
                        }
                    }
                }
            }

            dbus_message_iter_next(&array_iter);
        }

        dbus_message_unref(reply);
        return ssids;
    }
};

} // namespace network_mgr
} // namespace uli

#endif // ULI_NETWORK_MGR_IWD_BACKEND_HPP
