#ifndef ULI_NETWORK_MGR_WPA_BACKEND_HPP
#define ULI_NETWORK_MGR_WPA_BACKEND_HPP

/*
 * WPA Supplicant Backend (wpa_backend.hpp)
 * =========================================
 * Communicates with wpa_supplicant to scan and connect to wireless networks.
 *
 * STRATEGY:
 *   - Detection and interface discovery use NATIVE D-BUS via DBusPoint
 *     (talking to fi.w1.wpa_supplicant1).
 *   - Scanning uses D-Bus: call Scan() on the interface object.
 *   - Network listing reads BSSs property and extracts SSIDs via D-Bus.
 *   - Connection uses D-Bus: AddNetwork + SelectNetwork with PSK dict.
 *     Unlike IWD, wpa_supplicant accepts PSK directly in the AddNetwork
 *     dict, so full native D-Bus connection IS possible.
 *   - If ANY D-Bus call fails, the operation falls back to popen("wpa_cli").
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

class WpaBackend {
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
     * Checks if wpa_supplicant is running by asking D-Bus if
     * "fi.w1.wpa_supplicant1" is registered. Falls back to `which wpa_cli`.
     */
    static bool is_available() {
        if (uli::dbus::DBusPoint::is_service_active("fi.w1.wpa_supplicant1")) {
            return true;
        }
        std::string res = exec_command("which wpa_cli 2>/dev/null");
        return !res.empty();
    }

    /*
     * get_wireless_interfaces()
     * --------------------------
     * Discovers wireless interface names managed by wpa_supplicant.
     *
     * D-Bus flow:
     *   1. Read property "Interfaces" from fi.w1.wpa_supplicant1
     *      at path /fi/w1/wpa_supplicant1
     *      → Returns: ARRAY of OBJECT_PATH
     *        e.g. ["/fi/w1/wpa_supplicant1/Interfaces/0"]
     *
     *   2. For each interface path, read property "Ifname":
     *      Properties.Get("fi.w1.wpa_supplicant1.Interface", "Ifname")
     *      → Returns the interface name string e.g. "wlan0"
     *
     * Fallback: parse `iw dev` output.
     */
    static std::vector<std::string> get_wireless_interfaces() {
        std::vector<std::string> devices;

        // Try D-Bus: enumerate wpa_supplicant managed interfaces
        auto iface_paths = get_wpa_interface_paths();
        for (const auto& path : iface_paths) {
            std::string name = uli::dbus::DBusPoint::get_string_property(
                "fi.w1.wpa_supplicant1", path,
                "fi.w1.wpa_supplicant1.Interface", "Ifname");
            if (!name.empty()) devices.push_back(name);
        }

        if (!devices.empty()) return devices;

        // Fallback: parse `iw dev`
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
     *   1. Find the wpa_supplicant interface object path for this iface
     *   2. Call Scan(dict) on fi.w1.wpa_supplicant1.Interface:
     *      destination:  "fi.w1.wpa_supplicant1"
     *      path:         (interface object path)
     *      interface:    "fi.w1.wpa_supplicant1.Interface"
     *      method:       "Scan"
     *      argument:     DICT { "Type" → "active" }
     *
     * Fallback: `wpa_cli -i <iface> scan`
     */
    static void scan_interface(const std::string& iface) {
        std::string iface_path = find_wpa_interface_path(iface);

        if (!iface_path.empty()) {
            // Construct Scan method call with {Type: active} dict
            DBusConnection* conn = uli::dbus::DBusPoint::get_connection();
            if (conn) {
                DBusMessage* msg = dbus_message_new_method_call(
                    "fi.w1.wpa_supplicant1",
                    iface_path.c_str(),
                    "fi.w1.wpa_supplicant1.Interface",
                    "Scan"
                );

                if (msg) {
                    /*
                     * Scan() takes a dict argument: a{sv}
                     * We send { "Type" => VARIANT("active") }
                     * This tells wpa_supplicant to do an active probe scan.
                     */
                    DBusMessageIter iter, dict_iter, entry_iter, variant_iter;
                    dbus_message_iter_init_append(msg, &iter);
                    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);
                    dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry_iter);

                    const char* key = "Type";
                    dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &key);
                    dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
                    const char* val = "active";
                    dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &val);
                    dbus_message_iter_close_container(&entry_iter, &variant_iter);

                    dbus_message_iter_close_container(&dict_iter, &entry_iter);
                    dbus_message_iter_close_container(&iter, &dict_iter);

                    DBusError err;
                    dbus_error_init(&err);
                    DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn, msg, 10000, &err);
                    dbus_message_unref(msg);
                    if (reply) {
                        dbus_message_unref(reply);
                        if (!dbus_error_is_set(&err)) return; // D-Bus scan succeeded
                    }
                    if (dbus_error_is_set(&err)) dbus_error_free(&err);
                }
            }
        }

        // Fallback to CLI
        std::string cmd = "wpa_cli -i " + iface + " scan";
        exec_command(cmd.c_str());
    }

    /*
     * get_available_networks(iface)
     * ------------------------------
     * Returns a list of SSIDs visible on the given interface.
     *
     * D-Bus flow:
     *   1. Find the interface object path for this iface
     *   2. Read property "BSSs" from fi.w1.wpa_supplicant1.Interface
     *      → Returns: ARRAY of OBJECT_PATH
     *        e.g. ["/fi/w1/wpa_supplicant1/Interfaces/0/BSSs/0", ...]
     *   3. For each BSS path, read property "SSID":
     *      Properties.Get("fi.w1.wpa_supplicant1.BSS", "SSID")
     *      → Returns: ARRAY of BYTE (raw SSID bytes)
     *      We convert the byte array to a string.
     *
     * Fallback: parse `wpa_cli scan_results` output.
     */
    static std::vector<std::string> get_available_networks(const std::string& iface) {
        std::vector<std::string> ssids;

        std::string iface_path = find_wpa_interface_path(iface);
        if (!iface_path.empty()) {
            ssids = get_bss_ssids_via_dbus(iface_path);
            if (!ssids.empty()) return ssids;
        }

        // Fallback: parse wpa_cli scan_results
        std::string cmd = "wpa_cli -i " + iface + " scan_results";
        std::string out = exec_command(cmd.c_str());

        size_t pos = 0;
        bool header_skipped = false;
        while ((pos = out.find('\n')) != std::string::npos) {
            std::string line = out.substr(0, pos);
            out.erase(0, pos + 1);
            if (!header_skipped) { header_skipped = true; continue; } // skip header

            // wpa_cli scan_results format: bssid / freq / signal / flags / ssid
            // Fields are tab-separated
            size_t tab_count = 0;
            size_t last_tab = 0;
            for (size_t i = 0; i < line.size(); i++) {
                if (line[i] == '\t') {
                    tab_count++;
                    if (tab_count == 4) { last_tab = i + 1; break; }
                }
            }
            if (tab_count >= 4 && last_tab < line.size()) {
                std::string ssid = line.substr(last_tab);
                ssid.erase(ssid.find_last_not_of(" \n\r\t") + 1);
                if (!ssid.empty() && std::find(ssids.begin(), ssids.end(), ssid) == ssids.end()) {
                    ssids.push_back(ssid);
                }
            }
        }
        return ssids;
    }

    /*
     * connect_network(iface, ssid, password)
     * ----------------------------------------
     * Connects to a wireless network via wpa_supplicant.
     *
     * D-Bus flow:
     *   1. Call AddNetwork(dict) on the interface:
     *      dict = { "ssid" => SSID, "psk" => password } (or key_mgmt=NONE for open)
     *      → Returns: OBJECT_PATH of the new network
     *   2. Call SelectNetwork(network_path) on the interface
     *
     * Unlike IWD, wpa_supplicant accepts the PSK directly in the AddNetwork
     * dict, so we can do full native D-Bus connection here.
     *
     * Fallback: use wpa_cli add_network / set_network / select_network.
     */
    static bool connect_network(const std::string& iface, const std::string& ssid, const std::string& password) {
        std::string iface_path = find_wpa_interface_path(iface);

        if (!iface_path.empty()) {
            bool ok = connect_via_dbus(iface_path, ssid, password);
            if (ok) return true;
        }

        // Fallback to CLI
        std::string add_net = "wpa_cli -i " + iface + " add_network";
        std::string net_id = exec_command(add_net.c_str());
        net_id.erase(net_id.find_last_not_of(" \n\r\t") + 1);

        if (net_id.empty() || net_id.find("FAIL") != std::string::npos) return false;

        std::string set_ssid = "wpa_cli -i " + iface + " set_network " + net_id + " ssid '\"" + ssid + "\"'";
        exec_command(set_ssid.c_str());

        if (password.empty() || password == "None") {
            std::string set_key = "wpa_cli -i " + iface + " set_network " + net_id + " key_mgmt NONE";
            exec_command(set_key.c_str());
        } else {
            std::string set_psk = "wpa_cli -i " + iface + " set_network " + net_id + " psk '\"" + password + "\"'";
            exec_command(set_psk.c_str());
        }

        std::string select_net = "wpa_cli -i " + iface + " select_network " + net_id;
        exec_command(select_net.c_str());

        std::string enable_net = "wpa_cli -i " + iface + " enable_network " + net_id;
        exec_command(enable_net.c_str());

        return true;
    }

private:
    /*
     * get_wpa_interface_paths()
     * --------------------------
     * Internal: discovers all wpa_supplicant managed interface object paths
     * by introspecting the wpa_supplicant root and finding paths that
     * implement fi.w1.wpa_supplicant1.Interface.
     */
    static std::vector<std::string> get_wpa_interface_paths() {
        return uli::dbus::DBusPoint::find_objects_with_interface(
            "fi.w1.wpa_supplicant1",
            "/fi/w1/wpa_supplicant1",
            "fi.w1.wpa_supplicant1.Interface"
        );
    }

    /*
     * find_wpa_interface_path(iface)
     * --------------------------------
     * Internal: finds the D-Bus object path for a specific interface name.
     */
    static std::string find_wpa_interface_path(const std::string& iface) {
        auto paths = get_wpa_interface_paths();
        for (const auto& path : paths) {
            std::string name = uli::dbus::DBusPoint::get_string_property(
                "fi.w1.wpa_supplicant1", path,
                "fi.w1.wpa_supplicant1.Interface", "Ifname");
            if (name == iface) return path;
        }
        return "";
    }

    /*
     * get_bss_ssids_via_dbus(iface_path)
     * -------------------------------------
     * Internal: reads BSS objects from wpa_supplicant and extracts SSIDs.
     *
     * The BSSs property returns an array of object paths. For each BSS,
     * the SSID property is an array of bytes. We convert to string.
     *
     * Note: We use introspection to find BSS child objects since reading
     * the BSSs property requires variant array parsing which is complex.
     */
    static std::vector<std::string> get_bss_ssids_via_dbus(const std::string& iface_path) {
        std::vector<std::string> ssids;

        // Find BSS child objects under the interface path
        auto bss_paths = uli::dbus::DBusPoint::find_objects_with_interface(
            "fi.w1.wpa_supplicant1", iface_path,
            "fi.w1.wpa_supplicant1.BSS");

        for (const auto& bss_path : bss_paths) {
            // Read SSID property — it's an array of bytes, but we can try
            // reading it as a string property first (some implementations)
            // If that fails, we'd need byte-array parsing.
            std::string ssid = read_bss_ssid(bss_path);
            if (!ssid.empty()) {
                if (std::find(ssids.begin(), ssids.end(), ssid) == ssids.end()) {
                    ssids.push_back(ssid);
                }
            }
        }

        return ssids;
    }

    /*
     * read_bss_ssid(bss_path)
     * -------------------------
     * Internal: reads the SSID from a BSS object. The SSID is stored as
     * an array of bytes (ay) in D-Bus. We read the property variant,
     * extract the byte array, and convert to a UTF-8 string.
     */
    static std::string read_bss_ssid(const std::string& bss_path) {
        DBusConnection* conn = uli::dbus::DBusPoint::get_connection();
        if (!conn) return "";

        DBusMessage* msg = dbus_message_new_method_call(
            "fi.w1.wpa_supplicant1",
            bss_path.c_str(),
            "org.freedesktop.DBus.Properties",
            "Get"
        );
        if (!msg) return "";

        const char* iface = "fi.w1.wpa_supplicant1.BSS";
        const char* prop = "SSID";
        dbus_message_append_args(msg,
            DBUS_TYPE_STRING, &iface,
            DBUS_TYPE_STRING, &prop,
            DBUS_TYPE_INVALID);

        DBusError err;
        dbus_error_init(&err);
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn, msg, 3000, &err);
        dbus_message_unref(msg);

        if (dbus_error_is_set(&err)) { dbus_error_free(&err); return ""; }
        if (!reply) return "";

        // Reply is VARIANT -> ARRAY of BYTE
        DBusMessageIter iter;
        dbus_message_iter_init(reply, &iter);

        if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_VARIANT) {
            dbus_message_unref(reply);
            return "";
        }

        DBusMessageIter variant_iter;
        dbus_message_iter_recurse(&iter, &variant_iter);

        if (dbus_message_iter_get_arg_type(&variant_iter) != DBUS_TYPE_ARRAY) {
            dbus_message_unref(reply);
            return "";
        }

        DBusMessageIter array_iter;
        dbus_message_iter_recurse(&variant_iter, &array_iter);

        std::string ssid;
        while (dbus_message_iter_get_arg_type(&array_iter) == DBUS_TYPE_BYTE) {
            unsigned char byte;
            dbus_message_iter_get_basic(&array_iter, &byte);
            if (byte != 0) ssid += static_cast<char>(byte);
            dbus_message_iter_next(&array_iter);
        }

        dbus_message_unref(reply);
        return ssid;
    }

    /*
     * connect_via_dbus(iface_path, ssid, password)
     * -----------------------------------------------
     * Internal: uses wpa_supplicant D-Bus API to add and select a network.
     *
     * D-Bus call: AddNetwork(a{sv})
     *   dict entries:
     *     "ssid"    → STRING ssid
     *     "psk"     → STRING password   (or "key_mgmt" → "NONE" for open)
     *
     * Then: SelectNetwork(OBJECT_PATH new_network_path)
     */
    static bool connect_via_dbus(const std::string& iface_path, const std::string& ssid, const std::string& password) {
        DBusConnection* conn = uli::dbus::DBusPoint::get_connection();
        if (!conn) return false;

        // Step 1: AddNetwork
        DBusMessage* msg = dbus_message_new_method_call(
            "fi.w1.wpa_supplicant1",
            iface_path.c_str(),
            "fi.w1.wpa_supplicant1.Interface",
            "AddNetwork"
        );
        if (!msg) return false;

        DBusMessageIter iter, dict_iter;
        dbus_message_iter_init_append(msg, &iter);
        dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict_iter);

        // Add "ssid" entry
        {
            DBusMessageIter entry_iter, variant_iter;
            dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry_iter);
            const char* key = "ssid";
            dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &key);
            dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
            const char* val = ssid.c_str();
            dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &val);
            dbus_message_iter_close_container(&entry_iter, &variant_iter);
            dbus_message_iter_close_container(&dict_iter, &entry_iter);
        }

        if (password.empty() || password == "None") {
            // Open network: key_mgmt = NONE
            DBusMessageIter entry_iter, variant_iter;
            dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry_iter);
            const char* key = "key_mgmt";
            dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &key);
            dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
            const char* val = "NONE";
            dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &val);
            dbus_message_iter_close_container(&entry_iter, &variant_iter);
            dbus_message_iter_close_container(&dict_iter, &entry_iter);
        } else {
            // PSK network: psk = password
            DBusMessageIter entry_iter, variant_iter;
            dbus_message_iter_open_container(&dict_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &entry_iter);
            const char* key = "psk";
            dbus_message_iter_append_basic(&entry_iter, DBUS_TYPE_STRING, &key);
            dbus_message_iter_open_container(&entry_iter, DBUS_TYPE_VARIANT, "s", &variant_iter);
            const char* val = password.c_str();
            dbus_message_iter_append_basic(&variant_iter, DBUS_TYPE_STRING, &val);
            dbus_message_iter_close_container(&entry_iter, &variant_iter);
            dbus_message_iter_close_container(&dict_iter, &entry_iter);
        }

        dbus_message_iter_close_container(&iter, &dict_iter);

        DBusError err;
        dbus_error_init(&err);
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn, msg, 5000, &err);
        dbus_message_unref(msg);

        if (dbus_error_is_set(&err)) { dbus_error_free(&err); return false; }
        if (!reply) return false;

        // Extract the new network object path from the reply
        const char* net_path = nullptr;
        dbus_message_get_args(reply, &err,
            DBUS_TYPE_OBJECT_PATH, &net_path,
            DBUS_TYPE_INVALID);
        dbus_message_unref(reply);

        if (dbus_error_is_set(&err) || !net_path) {
            if (dbus_error_is_set(&err)) dbus_error_free(&err);
            return false;
        }

        // Step 2: SelectNetwork
        DBusMessage* sel_msg = dbus_message_new_method_call(
            "fi.w1.wpa_supplicant1",
            iface_path.c_str(),
            "fi.w1.wpa_supplicant1.Interface",
            "SelectNetwork"
        );
        if (!sel_msg) return false;

        dbus_message_append_args(sel_msg,
            DBUS_TYPE_OBJECT_PATH, &net_path,
            DBUS_TYPE_INVALID);

        reply = dbus_connection_send_with_reply_and_block(conn, sel_msg, 5000, &err);
        dbus_message_unref(sel_msg);

        if (dbus_error_is_set(&err)) { dbus_error_free(&err); return false; }
        if (reply) dbus_message_unref(reply);

        return true;
    }
};

} // namespace network_mgr
} // namespace uli

#endif // ULI_NETWORK_MGR_WPA_BACKEND_HPP
