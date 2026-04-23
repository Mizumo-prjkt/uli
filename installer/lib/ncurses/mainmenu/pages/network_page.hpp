#pragma once
// network_page.hpp - Network Configuration page
// Allows selecting interfaces, connecting/disconnecting, WiFi SSID selection,
// DHCP/Static IP configuration, and hostname setting.
#include "page.hpp"
#include "../../ncurseslib.hpp"
#include <vector>
#include <string>

struct NetIfaceInfo {
    std::string name;
    std::string type;        // "Ethernet" or "Wireless"
    bool        connected;
    std::string ip_address;
    std::string mac_address;
    std::string gateway;
    std::string dns;
    bool        use_dhcp;
};

struct WifiNetwork {
    std::string ssid;
    int         signal;      // 0-100
    bool        secured;
    bool        connected;
};

class NetworkPage : public Page {
    std::vector<NetIfaceInfo> ifaces_;
    std::vector<WifiNetwork> wifi_networks_;
    int selected_ = 0;
    std::string hostname_ = "archlinux";
    // 0=interface list, 1=hostname, 2=actions, 3=wifi list, 4=ip config
    int focus_ = 0;
    int action_idx_ = 0;
    int wifi_selected_ = 0;
    int wifi_scroll_ = 0;
    int ip_field_ = 0; // 0=dhcp toggle, 1=ip, 2=gateway, 3=dns

public:
    NetworkPage(const std::vector<NetIfaceInfo>& ifaces,
                const std::vector<WifiNetwork>& wifi)
        : ifaces_(ifaces), wifi_networks_(wifi) {}

    std::string title() const override { return "Network Configuration"; }

    void render(WINDOW* win) override {
        int h, w;
        getmaxyx(win, h, w);
        auto& sel_iface = ifaces_[selected_];

        // ── Hostname ──
        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Network Configuration");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        if (focus_ == 1) {
            wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
            mvwhline(win, 2, 2, ' ', w - 4);
        }
        mvwprintw(win, 2, 2, "Hostname: ");
        if (focus_ == 1)
            wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));

        wattron(win, COLOR_PAIR(CP_INPUT_FIELD));
        mvwprintw(win, 2, 12, " %-20s ", hostname_.c_str());
        wattroff(win, COLOR_PAIR(CP_INPUT_FIELD));

        NcursesLib::draw_hline(win, 3, 1, w - 2);

        // ── Interface table ──
        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 4, 2, "Network Interfaces");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        wattron(win, COLOR_PAIR(CP_TABLE_HEADER) | A_BOLD | A_UNDERLINE);
        mvwprintw(win, 5, 3, " %-10s %-10s %-8s %-16s %-18s",
            "Interface", "Type", "Status", "IP Address", "MAC Address");
        wattroff(win, COLOR_PAIR(CP_TABLE_HEADER) | A_BOLD | A_UNDERLINE);

        for (int i = 0; i < (int)ifaces_.size() && 6 + i < h - 14; i++) {
            auto& iface = ifaces_[i];
            int y = 6 + i;

            if (focus_ == 0 && i == selected_) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y, 2, ' ', w - 4);
            } else {
                wattron(win, COLOR_PAIR(CP_NORMAL));
            }

            mvwprintw(win, y, 3, " %-10s %-10s",
                iface.name.c_str(), iface.type.c_str());

            if (focus_ == 0 && i == selected_)
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));

            int scp = iface.connected ? CP_CHECKBOX_ON : CP_CHECKBOX_OFF;
            wattron(win, COLOR_PAIR(scp) | A_BOLD);
            mvwprintw(win, y, 26, "%-8s", iface.connected ? "UP" : "DOWN");
            wattroff(win, COLOR_PAIR(scp) | A_BOLD);

            int cp = (focus_ == 0 && i == selected_) ? CP_HIGHLIGHT : CP_NORMAL;
            wattron(win, COLOR_PAIR(cp));
            mvwprintw(win, y, 34, "%-16s %-18s",
                iface.ip_address.empty() ? "-" : iface.ip_address.c_str(),
                iface.mac_address.c_str());
            wattroff(win, COLOR_PAIR(cp));
        }

        int section_y = 6 + (int)ifaces_.size() + 1;

        // ── Actions for selected interface ──
        NcursesLib::draw_hline(win, section_y - 1, 1, w - 2);

        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, section_y, 2, "Actions for: %s (%s)",
            sel_iface.name.c_str(), sel_iface.type.c_str());
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        const char* actions[] = {
            sel_iface.connected ? "Disconnect" : "Connect",
            "Configure IP",
            sel_iface.type == "Wireless" ? "Scan WiFi Networks" : "(Ethernet - no WiFi scan)",
        };
        int num_actions = 3;

        for (int i = 0; i < num_actions; i++) {
            int ay = section_y + 1 + i;
            if (ay >= h - 8) break;
            bool disabled = (i == 2 && sel_iface.type != "Wireless");
            if (focus_ == 2 && i == action_idx_) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, ay, 3, ' ', w - 6);
                mvwprintw(win, ay, 4, "%s", actions[i]);
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(disabled ? CP_SEPARATOR : CP_NORMAL));
                mvwprintw(win, ay, 4, "%s", actions[i]);
                wattroff(win, COLOR_PAIR(disabled ? CP_SEPARATOR : CP_NORMAL));
            }
        }

        int detail_y = section_y + 1 + num_actions + 1;

        // ── WiFi scan results (if wireless selected) ──
        if (sel_iface.type == "Wireless" && !wifi_networks_.empty()) {
            NcursesLib::draw_hline(win, detail_y - 1, 1, w - 2);
            wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
            mvwprintw(win, detail_y, 2, "Available WiFi Networks");
            wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

            wattron(win, COLOR_PAIR(CP_TABLE_HEADER) | A_BOLD | A_UNDERLINE);
            mvwprintw(win, detail_y + 1, 3, " %-24s %-6s %-8s %-10s",
                "SSID", "Signal", "Security", "Status");
            wattroff(win, COLOR_PAIR(CP_TABLE_HEADER) | A_BOLD | A_UNDERLINE);

            int wifi_list_h = h - detail_y - 4;
            if (wifi_selected_ < wifi_scroll_) wifi_scroll_ = wifi_selected_;
            if (wifi_selected_ >= wifi_scroll_ + wifi_list_h) wifi_scroll_ = wifi_selected_ - wifi_list_h + 1;

            for (int i = 0; i < wifi_list_h && (wifi_scroll_ + i) < (int)wifi_networks_.size(); i++) {
                int idx = wifi_scroll_ + i;
                auto& net = wifi_networks_[idx];
                int wy = detail_y + 2 + i;

                if (focus_ == 3 && idx == wifi_selected_) {
                    wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                    mvwhline(win, wy, 2, ' ', w - 4);
                } else {
                    wattron(win, COLOR_PAIR(CP_NORMAL));
                }

                std::string quality;
                if (net.signal >= 80) quality = "Excellent";
                else if (net.signal >= 60) quality = "Good";
                else if (net.signal >= 40) quality = "Fair";
                else quality = "Poor";

                mvwprintw(win, wy, 3, " %-24s", net.ssid.c_str());

                if (focus_ == 3 && idx == wifi_selected_)
                    wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));

                // Color-coded signal percentage
                int sig_cp = net.signal >= 60 ? CP_CHECKBOX_ON : (net.signal >= 30 ? CP_ACTION_ITEM : CP_CHECKBOX_OFF);
                wattron(win, COLOR_PAIR(sig_cp) | A_BOLD);
                mvwprintw(win, wy, 28, "%3d%% [%-9s]", net.signal, quality.c_str());
                wattroff(win, COLOR_PAIR(sig_cp) | A_BOLD);

                int cp = (focus_ == 3 && idx == wifi_selected_) ? CP_HIGHLIGHT : CP_NORMAL;
                wattron(win, COLOR_PAIR(cp));
                mvwprintw(win, wy, 44, " %-8s %-10s",
                    net.secured ? "WPA2" : "Open",
                    net.connected ? "Connected" : "");
                wattroff(win, COLOR_PAIR(cp));
            }
            
            // Legend
            mvwprintw(win, h - 1, 2, "Signal: ");
            wattron(win, COLOR_PAIR(CP_CHECKBOX_ON));  waddstr(win, "Excellent/Good ");
            wattron(win, COLOR_PAIR(CP_ACTION_ITEM));   waddstr(win, "Fair ");
            wattron(win, COLOR_PAIR(CP_CHECKBOX_OFF)); waddstr(win, "Poor");
            wattroff(win, COLOR_PAIR(CP_CHECKBOX_OFF));
        }

        // ── IP Configuration panel (shown when "Configure IP" action is selected) ──
        if (focus_ == 4) {
            // Draw a sub-box overlay
            int box_h = 8;
            int box_w = 50;
            int box_y = (h - box_h) / 2;
            int box_x = (w - box_w) / 2;
            WINDOW* ipwin = derwin(win, box_h, box_w, box_y, box_x);
            NcursesLib::fill_background(ipwin, CP_NORMAL);
            NcursesLib::draw_titled_box(ipwin, "IP Configuration - " + sel_iface.name);

            const char* dhcp_label = sel_iface.use_dhcp ? "[x] DHCP" : "[ ] DHCP (Static)";
            int fc;

            fc = (ip_field_ == 0) ? CP_HIGHLIGHT : CP_NORMAL;
            wattron(ipwin, COLOR_PAIR(fc));
            mvwprintw(ipwin, 1, 2, "%-46s", dhcp_label);
            wattroff(ipwin, COLOR_PAIR(fc));

            fc = (ip_field_ == 1) ? CP_HIGHLIGHT : CP_NORMAL;
            wattron(ipwin, COLOR_PAIR(fc));
            mvwprintw(ipwin, 2, 2, "IP Address:  %-33s",
                sel_iface.ip_address.empty() ? "(not set)" : sel_iface.ip_address.c_str());
            wattroff(ipwin, COLOR_PAIR(fc));

            fc = (ip_field_ == 2) ? CP_HIGHLIGHT : CP_NORMAL;
            wattron(ipwin, COLOR_PAIR(fc));
            mvwprintw(ipwin, 3, 2, "Gateway:     %-33s",
                sel_iface.gateway.empty() ? "(not set)" : sel_iface.gateway.c_str());
            wattroff(ipwin, COLOR_PAIR(fc));

            fc = (ip_field_ == 3) ? CP_HIGHLIGHT : CP_NORMAL;
            wattron(ipwin, COLOR_PAIR(fc));
            mvwprintw(ipwin, 4, 2, "DNS:         %-33s",
                sel_iface.dns.empty() ? "(not set)" : sel_iface.dns.c_str());
            wattroff(ipwin, COLOR_PAIR(fc));

            mvwprintw(ipwin, 6, 2, "ENTER=edit  SPACE=toggle DHCP  ESC=close");
            delwin(ipwin);
        }
    }

    bool handle_input(WINDOW* win, int ch) override {
        if (focus_ == 4) {
            // IP config overlay
            if (NcursesLib::is_back_key(ch)) { focus_ = 2; return true; }
            if (ch == KEY_UP && ip_field_ > 0) { ip_field_--; return true; }
            if (ch == KEY_DOWN && ip_field_ < 3) { ip_field_++; return true; }
            if (ip_field_ == 0 && (ch == ' ' || ch == '\n')) {
                ifaces_[selected_].use_dhcp = !ifaces_[selected_].use_dhcp;
                return true;
            }
            if (ch == '\n' || ch == KEY_ENTER) {
                auto& iface = ifaces_[selected_];
                int h_win, w_win;
                getmaxyx(win, h_win, w_win);
                int box_y = (h_win - 8) / 2;
                int box_x = (w_win - 50) / 2;
                if (ip_field_ == 1) {
                    std::string val = NcursesLib::text_input(win, box_y + 2, box_x + 15, 15);
                    if (!val.empty()) iface.ip_address = val;
                } else if (ip_field_ == 2) {
                    std::string val = NcursesLib::text_input(win, box_y + 3, box_x + 15, 15);
                    if (!val.empty()) iface.gateway = val;
                } else if (ip_field_ == 3) {
                    std::string val = NcursesLib::text_input(win, box_y + 4, box_x + 15, 15);
                    if (!val.empty()) iface.dns = val;
                }
                return true;
            }
            return true;
        }

        if (ch == '\t') {
            // Cycle: interfaces → hostname → actions → wifi (if wireless)
            int max_focus = 2;
            if (ifaces_[selected_].type == "Wireless" && !wifi_networks_.empty())
                max_focus = 3;
            focus_ = (focus_ + 1) % (max_focus + 1);
            return true;
        }

        if (focus_ == 0) {
            if (ch == KEY_UP && selected_ > 0) { selected_--; return true; }
            if (ch == KEY_DOWN && selected_ < (int)ifaces_.size() - 1) { selected_++; return true; }
            if (ch == '\n' || ch == KEY_ENTER) { focus_ = 2; return true; }
            return false;
        }
        if (focus_ == 1) {
            if (ch == '\n' || ch == KEY_ENTER) {
                std::string input = NcursesLib::text_input(win, 2, 13, 20);
                if (!input.empty()) hostname_ = input;
                return true;
            }
            return false;
        }
        if (focus_ == 2) {
            if (ch == KEY_UP && action_idx_ > 0) { action_idx_--; return true; }
            if (ch == KEY_DOWN && action_idx_ < 2) { action_idx_++; return true; }
            if (ch == '\n' || ch == KEY_ENTER) {
                auto& iface = ifaces_[selected_];
                if (action_idx_ == 0) {
                    // Connect/Disconnect toggle
                    iface.connected = !iface.connected;
                    if (iface.connected && iface.ip_address.empty())
                        iface.ip_address = "192.168.1." + std::to_string(100 + selected_);
                    if (!iface.connected) iface.ip_address = "";
                } else if (action_idx_ == 1) {
                    // Open IP config overlay
                    focus_ = 4;
                    ip_field_ = 0;
                } else if (action_idx_ == 2 && iface.type == "Wireless") {
                    // Switch to wifi list
                    focus_ = 3;
                    wifi_selected_ = 0;
                }
                return true;
            }
            return false;
        }
        if (focus_ == 3) {
            // WiFi network list
            if (ch == KEY_UP && wifi_selected_ > 0) { wifi_selected_--; return true; }
            if (ch == KEY_DOWN && wifi_selected_ < (int)wifi_networks_.size() - 1) { wifi_selected_++; return true; }
            if (ch == '\n' || ch == KEY_ENTER) {
                // "Connect" to selected WiFi (simulated)
                for (auto& n : wifi_networks_) n.connected = false;
                wifi_networks_[wifi_selected_].connected = true;
                auto& iface = ifaces_[selected_];
                iface.connected = true;
                iface.ip_address = "192.168.1." + std::to_string(100 + wifi_selected_);
                return true;
            }
            return false;
        }
        return false;
    }
};
