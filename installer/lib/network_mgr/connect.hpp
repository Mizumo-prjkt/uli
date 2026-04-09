#ifndef ULI_NETWORK_MGR_CONNECT_HPP
#define ULI_NETWORK_MGR_CONNECT_HPP

#include <iostream>
#include <string>
#include <array>
#include <memory>
#include <cstdio>
#include "iwd_backend.hpp"
#include "wpa_backend.hpp"
#include "../runtime/dialogbox.hpp"
#include <chrono>
#include <thread>

namespace uli {
namespace network_mgr {

class NetworkConnectHelper {
public:
    /*
     * exec_command() — small popen helper for verification commands.
     */
    static std::string exec_command(const char* cmd) {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe) return "";
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    /*
     * get_default_gateway()
     * ----------------------
     * Discovers the default gateway IP by parsing the kernel routing table.
     *
     * Reads: `ip route show default`
     * Example output: "default via 192.168.1.1 dev wlan0 proto dhcp metric 600"
     * We extract the IP after "via".
     *
     * Returns: gateway IP string, or "" if none found.
     */
    static std::string get_default_gateway() {
        std::string output = exec_command("ip route show default 2>/dev/null");
        // Parse "default via X.X.X.X ..."
        std::string marker = "via ";
        size_t pos = output.find(marker);
        if (pos == std::string::npos) return "";
        
        pos += marker.length();
        size_t end = output.find(' ', pos);
        if (end == std::string::npos) end = output.find('\n', pos);
        if (end == std::string::npos) end = output.length();
        
        std::string gw = output.substr(pos, end - pos);
        gw.erase(gw.find_last_not_of(" \t\r\n") + 1);
        return gw;
    }

    /*
     * ping_host(host, timeout_sec)
     * -----------------------------
     * Sends a single ICMP ping to the given host and returns true if
     * a response is received within the timeout.
     *
     * Uses: `ping -c 1 -W <timeout> <host>`
     *   -c 1  = send 1 packet
     *   -W    = timeout in seconds
     *
     * Returns: true if exit code 0 (host responded), false otherwise.
     */
    static bool ping_host(const std::string& host, int timeout_sec = 3) {
        std::string cmd = "ping -c 1 -W " + std::to_string(timeout_sec) + " " + host + " >/dev/null 2>&1";
        int ret = system(cmd.c_str());
        return (ret == 0);
    }

    /*
     * verify_connection()
     * --------------------
     * Post-connection verification that checks:
     *
     *   1. GATEWAY PING: Discovers the default gateway (router) IP from
     *      the kernel routing table and pings it. This confirms we have
     *      a valid link-layer connection to the router and received a
     *      DHCP lease.
     *
     *   2. INTERNET PING: Pings 8.8.8.8 (Google DNS) to verify that
     *      the router is actually forwarding traffic to the internet.
     *
     * Returns a formatted status string shown to the user, and sets
     * the out parameter `connected` to true only if both checks pass.
     */
    static std::string verify_connection(bool& connected) {
        connected = false;
        std::string report;

        // ── Step 1: Discover and ping the gateway ──────────────
        std::string gateway = get_default_gateway();
        if (gateway.empty()) {
            report += "Gateway discovery:  FAILED (no default route found)\n";
            report += "  → DHCP may not have assigned an IP yet.\n";
            return report;
        }

        report += "Default gateway:    " + gateway + "\n";

        if (ping_host(gateway, 3)) {
            report += "Gateway ping:       OK\n";
        } else {
            report += "Gateway ping:       FAILED\n";
            report += "  → Router is not responding. Check passphrase or signal.\n";
            return report;
        }

        // ── Step 2: Ping external host for internet access ─────
        if (ping_host("8.8.8.8", 5)) {
            report += "Internet (8.8.8.8): OK\n";
            connected = true;
        } else {
            report += "Internet (8.8.8.8): FAILED\n";
            report += "  → Router reached, but no internet forwarding.\n";
        }

        return report;
    }

    /*
     * ui_setup_wireless_connection(connected_ssid)
     * -----------------------------------------------
     * Full UI flow with SSID output: backend detection → interface selection →
     * scan → SSID selection → passphrase → connect → VERIFY connection.
     * On success, sets connected_ssid to the name of the network we joined.
     */
    static bool ui_setup_wireless_connection(std::string& connected_ssid) {
        connected_ssid = "";
        std::vector<std::string> ifaces;
        bool use_iwd = false;
        
        if (IwdBackend::is_available()) {
            ifaces = IwdBackend::get_wireless_interfaces();
            use_iwd = true;
        } else if (WpaBackend::is_available()) {
            ifaces = WpaBackend::get_wireless_interfaces();
            use_iwd = false;
        } else {
            uli::runtime::DialogBox::show_alert("Network Manager", "No wireless backend (iwctl or wpa_cli) available on this system. Cannot configure Wi-Fi natively.");
            return false;
        }
        
        if (ifaces.empty()) {
            uli::runtime::DialogBox::show_alert("No Devices", "No wireless networking interfaces were found on this system (requires wlan capability).");
            return false;
        }
        
        std::string target_iface = ifaces[0];
        if (ifaces.size() > 1) {
            int r = uli::runtime::DialogBox::ask_selection("Interface", "Select Wireless Interface:", ifaces);
            if (r != -1) target_iface = ifaces[r];
            else return false;
        }
        
        // Command the scan
        if (use_iwd) IwdBackend::scan_interface(target_iface);
        else WpaBackend::scan_interface(target_iface);
        
        // Wait briefly for scan results to populate
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        std::vector<std::string> scanned;
        if (use_iwd) scanned = IwdBackend::get_available_networks(target_iface);
        else scanned = WpaBackend::get_available_networks(target_iface);
        
        if (scanned.empty()) {
            uli::runtime::DialogBox::show_alert("No Networks", "No wireless networks detected after scanning interface " + target_iface + ".");
            return false;
        }
        
        int sel = uli::runtime::DialogBox::ask_selection("Wireless Target", "Select Wireless Network (SSID) to connect:", scanned);
        if (sel == -1) return false;
        
        std::string ssid = scanned[sel];
        
        std::string pwd = uli::runtime::DialogBox::ask_password("Password", "Enter passphrase for '" + ssid + "' (Leave empty for open networks):");
        
        bool success = false;
        if (use_iwd) success = IwdBackend::connect_network(target_iface, ssid, pwd);
        else success = WpaBackend::connect_network(target_iface, ssid, pwd);
        
        if (!success) {
            uli::runtime::DialogBox::show_alert("Error", "Failed to dispatch connection to " + ssid + " on " + target_iface + ".");
            return false;
        }

        // ── Post-connection verification ────────────────────────
        uli::runtime::DialogBox::show_alert("Connecting", "Connection dispatched to '" + ssid + "'.\nWaiting for DHCP lease and link negotiation...");
        std::this_thread::sleep_for(std::chrono::seconds(5));

        bool connected = false;
        std::string report = verify_connection(connected);

        if (connected) {
            connected_ssid = ssid;
            uli::runtime::DialogBox::show_alert("Connection Verified", 
                "Successfully connected to '" + ssid + "'!\n\n" + report);
            return true;
        } else {
            uli::runtime::DialogBox::show_alert("Connection Verification Failed", 
                "Link to '" + ssid + "' could not be verified.\n\n" + report);
            return false;
        }
    }

    // ── Ethernet Detection ──────────────────────────────────────────

    struct EthernetInfo {
        std::string iface_name;    // e.g. "eth0", "enp3s0"
        bool is_active = false;
        std::string hardware_id;   // e.g. "[PCI 03:00.0] Realtek RTL8111"
        bool gateway_reachable = false;
        bool internet_reachable = false;
    };

    /*
     * get_ethernet_hw_id(iface_name)
     * --------------------------------
     * Discovers the hardware ID of an ethernet interface by reading the
     * sysfs device path and checking if it's PCI or USB.
     *
     * Sysfs walk:
     *   /sys/class/net/<iface>/device → symlink to bus device
     *   We readlink and parse:
     *     - PCI:  contains "/pci" → read PCI slot from basename (e.g. "0000:03:00.0")
     *             read /sys/class/net/<iface>/device/vendor+device for chip info
     *     - USB:  contains "/usb" → read USB ID similarly
     *
     * Also reads the device description from `lspci -s <slot>` or `lsusb`.
     */
    static std::string get_ethernet_hw_id(const std::string& iface_name) {
        // Read the device symlink
        std::string devpath_cmd = "readlink -f /sys/class/net/" + iface_name + "/device 2>/dev/null";
        std::string devpath = exec_command(devpath_cmd.c_str());
        devpath.erase(devpath.find_last_not_of(" \t\r\n") + 1);

        if (devpath.empty()) return "Unknown";

        if (devpath.find("/pci") != std::string::npos || devpath.find("/0000:") != std::string::npos) {
            // PCI device — extract slot from basename (e.g. "0000:03:00.0")
            std::string slot_cmd = "basename $(readlink -f /sys/class/net/" + iface_name + "/device) 2>/dev/null";
            std::string slot = exec_command(slot_cmd.c_str());
            slot.erase(slot.find_last_not_of(" \t\r\n") + 1);

            // Get description from lspci
            std::string desc_cmd = "lspci -s " + slot + " 2>/dev/null | cut -d: -f3-";
            std::string desc = exec_command(desc_cmd.c_str());
            desc.erase(desc.find_last_not_of(" \t\r\n") + 1);
            if (desc.empty()) desc = "PCI Ethernet Controller";

            // Strip the domain prefix for display (0000:03:00.0 → 03:00.0)
            std::string short_slot = slot;
            if (short_slot.size() > 5 && short_slot[4] == ':') {
                short_slot = short_slot.substr(5);
            }

            return "[PCI " + short_slot + "]" + desc;
        }
        else if (devpath.find("/usb") != std::string::npos) {
            // USB device — find the USB bus/device ID
            std::string vid_cmd = "cat /sys/class/net/" + iface_name + "/device/idVendor 2>/dev/null";
            std::string pid_cmd = "cat /sys/class/net/" + iface_name + "/device/idProduct 2>/dev/null";
            std::string vid = exec_command(vid_cmd.c_str());
            std::string pid = exec_command(pid_cmd.c_str());
            vid.erase(vid.find_last_not_of(" \t\r\n") + 1);
            pid.erase(pid.find_last_not_of(" \t\r\n") + 1);

            std::string usb_id = vid + ":" + pid;
            if (vid.empty() && pid.empty()) usb_id = "unknown";

            // Try to get description
            std::string desc_cmd = "lsusb -d " + usb_id + " 2>/dev/null | cut -d' ' -f7-";
            std::string desc = exec_command(desc_cmd.c_str());
            desc.erase(desc.find_last_not_of(" \t\r\n") + 1);
            if (desc.empty()) desc = "USB Ethernet Adapter";

            return "[USB " + usb_id + "] " + desc;
        }

        return "Unknown bus";
    }

    /*
     * detect_ethernet()
     * -------------------
     * Scans /sys/class/net for ethernet-class interfaces (type 1 = ARPHRD_ETHER)
     * that are NOT wireless (no /sys/class/net/<iface>/wireless directory).
     * For each, checks link state, gateway reachability, and internet pings.
     */
    static std::vector<EthernetInfo> detect_ethernet() {
        std::vector<EthernetInfo> results;

        // List network interfaces and filter for ethernet (not wireless, not loopback)
        std::string list_cmd = "ls /sys/class/net/ 2>/dev/null";
        std::string list_out = exec_command(list_cmd.c_str());

        size_t pos = 0;
        while ((pos = list_out.find('\n')) != std::string::npos) {
            std::string name = list_out.substr(0, pos);
            list_out.erase(0, pos + 1);
            if (name.empty() || name == "lo") continue;

            // Skip wireless interfaces (they have a /wireless subdir)
            std::string wireless_check = "test -d /sys/class/net/" + name + "/wireless && echo yes || echo no";
            std::string is_wireless = exec_command(wireless_check.c_str());
            is_wireless.erase(is_wireless.find_last_not_of(" \t\r\n") + 1);
            if (is_wireless == "yes") continue;

            // Check if interface type is ethernet (type == 1)
            std::string type_cmd = "cat /sys/class/net/" + name + "/type 2>/dev/null";
            std::string type_str = exec_command(type_cmd.c_str());
            type_str.erase(type_str.find_last_not_of(" \t\r\n") + 1);
            if (type_str != "1") continue;

            EthernetInfo info;
            info.iface_name = name;

            // Check operstate
            std::string state_cmd = "cat /sys/class/net/" + name + "/operstate 2>/dev/null";
            std::string state = exec_command(state_cmd.c_str());
            state.erase(state.find_last_not_of(" \t\r\n") + 1);
            info.is_active = (state == "up");

            // Hardware ID
            info.hardware_id = get_ethernet_hw_id(name);

            // Gateway + internet checks (only if active)
            if (info.is_active) {
                std::string gw = get_default_gateway();
                info.gateway_reachable = (!gw.empty() && ping_host(gw, 2));
                info.internet_reachable = ping_host("8.8.8.8", 3);
            }

            results.push_back(info);
        }
        return results;
    }

    /*
     * build_network_status_report(connected_ssid)
     * -----------------------------------------------
     * Builds a full network status report string showing:
     *   - All detected ethernet adapters with status, hw info, connectivity
     *   - Current wireless SSID if connected
     */
    /*
     * has_any_ethernet()
     * ------------------
     * Returns true if at least one ethernet-type interface is detected.
     */
    static bool has_any_ethernet() {
        return !detect_ethernet().empty();
    }

    /*
     * build_network_status_report(connected_ssid)
     * -----------------------------------------------
     * Builds a full network status report string showing:
     *   - All detected ethernet adapters with status, hw info, connectivity
     *   - Current wireless SSID if connected
     */
    static std::string build_network_status_report(const std::string& connected_ssid = "") {
        std::string report;

        auto ethernets = detect_ethernet();

        if (!ethernets.empty()) {
            for (const auto& eth : ethernets) {
                report += "── Ethernet Detected ──────────────────\n";
                report += "Status:                  " + std::string(eth.is_active ? "Active" : "Inactive") + "\n";
                if (!eth.is_active) {
                    report += "Note:                    Detected but not used\n";
                }
                report += "Net Identifier:          " + eth.iface_name + "\n";
                report += "Hardware ID:             " + eth.hardware_id + "\n";
                report += "Connected (Gateway):     " + std::string(eth.gateway_reachable ? "yes" : "no") + "\n";
                report += "Connected (via 8.8.8.8): " + std::string(eth.internet_reachable ? "yes" : "no") + "\n";
                report += "───────────────────────────────────────\n\n";
            }
        } else {
            report += "No ethernet interfaces detected.\n\n";
        }

        if (!connected_ssid.empty()) {
            report += "── Wireless ───────────────────────────\n";
            report += "SSID Name:               " + connected_ssid + "\n";
            report += "───────────────────────────────────────\n\n";
        }

        report += "Select an option to configure wireless or stay with wired.";

        return report;
    }

};

} // namespace network_mgr
} // namespace uli

#endif // ULI_NETWORK_MGR_CONNECT_HPP

