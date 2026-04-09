#ifndef ULI_TIMEDATE_UI_TZ_SELECT_HPP
#define ULI_TIMEDATE_UI_TZ_SELECT_HPP

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "../runtime/design_ui.hpp"
#include "../runtime/input_interpreter.hpp"
#include "../runtime/dialogbox.hpp"
#include "../runtime/menu.hpp"
#include "tz_manager.hpp"

namespace uli {
namespace timedate {

class UITzSetup {
public:
    static std::string run_live_search(const std::string& current_tz) {
        uli::runtime::InputInterpreter::enable_raw_mode();
        std::cout << "\033[?25l";

        bool running = true;
        std::string selected_tz = current_tz;
        std::vector<std::string> search_results = TzManager::get_all_timezones();
        
        std::string query = "";
        
        int focus_zone = 0; // 0=Search, 1=Result, 2=Dock
        int result_index = 0;
        int dock_index = 0; // 0=Save/Keep Current, 1=Cancel

        int visible_height = 10;

        while (running) {
            std::cout << "\033[H\033[J";
            uli::runtime::DesignUI::print_header("Timezone Configuration");
            std::cout << "\n";
            std::cout << "  Current Selection: \033[36m" << (selected_tz.empty() ? "(None)" : selected_tz) << "\033[0m\n\n";
            
            // Render Zone 0: Search Input
            std::cout << "  Search Timezone (Nation or City):\n";
            std::cout << "  +--------------------------------+\n";
            if (focus_zone == 0) {
                std::cout << "  + \033[42;30m" << query;
                int pad = 30 - query.length();
                if (pad < 0) pad = 0;
                for(int i = 0; i < pad; ++i) std::cout << " ";
                std::cout << "\033[0m +\n";
            } else {
                std::cout << "  + " << query;
                int pad = 30 - query.length();
                if (pad < 0) pad = 0;
                for(int i = 0; i < pad; ++i) std::cout << " ";
                std::cout << " +\n";
            }
            std::cout << "  +--------------------------------+\n\n";

            // Render Zone 1: Results Table
            std::string header  = "| Result                             |";
            std::string divider = "|------------------------------------|";
            
            std::cout << "  " << header << "\n";
            std::cout << "  " << divider << "\n";

            int start_res = 0;
            if (result_index >= visible_height) start_res = result_index - visible_height + 1;
            
            for (int i = 0; i < visible_height; ++i) {
                int r_idx = start_res + i;
                
                std::string row_str = "|                                    |";
                if (r_idx < static_cast<int>(search_results.size())) {
                    std::string opt_text = search_results[r_idx];
                    if (opt_text.length() > 34) opt_text = opt_text.substr(0, 31) + "...";
                    std::string inner = opt_text;
                    while (inner.length() < 34) inner += " ";
                    
                    if (focus_zone == 1 && r_idx == result_index) {
                        row_str = "| \033[42;30m" + inner + "\033[0m |";
                    } else if (opt_text == selected_tz) {
                        row_str = "| \033[36m" + inner + "\033[0m |"; // Highlight current in cyan
                    } else {
                        row_str = "| " + inner + " |";
                    }
                }
                
                std::cout << "  " << row_str << "\n";
            }
            std::cout << "  " << divider << "\n\n";

            // Render Zone 2: Dock
            std::vector<std::string> dock_options = {"Save / Keep Current", "Cancel"};
            std::cout << "  ";
            for (size_t i = 0; i < dock_options.size(); ++i) {
                if (focus_zone == 2 && static_cast<int>(i) == dock_index) {
                    std::cout << "\033[42;30m[" << dock_options[i] << "]\033[0m ";
                } else {
                    std::cout << "[" << dock_options[i] << "] ";
                }
            }
            std::cout << "\n\n  \033[90mPress TAB key to navigate through the UI\033[0m\n";
            
            // Input Mode logic
            if (focus_zone == 0) {
                char c = 0;
                uli::runtime::InputInterpreter::Key k = uli::runtime::InputInterpreter::read_key(c, true); 
                if (c == '\t') {
                    focus_zone = 1;
                } else if (k == uli::runtime::InputInterpreter::Key::ENTER) {
                    focus_zone = 1; // Drop into table on enter
                } else if (k == uli::runtime::InputInterpreter::Key::BACKSPACE) {
                    if (!query.empty()) {
                        query.pop_back();
                        if (query.empty()) search_results = TzManager::get_all_timezones();
                        else search_results = TzManager::fuzzy_search(query);
                    }
                    result_index = 0;
                } else if (k == uli::runtime::InputInterpreter::Key::OTHER) {
                    if (c >= 32 && c <= 126 && query.length() < 30) {
                        query += c;
                        search_results = TzManager::fuzzy_search(query);
                        result_index = 0;
                    }
                }
            } else {
                uli::runtime::InputInterpreter::Key k = uli::runtime::InputInterpreter::read_key();
                switch (k) {
                    case uli::runtime::InputInterpreter::Key::UP:
                        if (focus_zone == 1 && result_index > 0) result_index--;
                        break;
                    case uli::runtime::InputInterpreter::Key::DOWN:
                        if (focus_zone == 1 && result_index < static_cast<int>(search_results.size()) - 1) result_index++;
                        break;
                    case uli::runtime::InputInterpreter::Key::LEFT:
                        if (focus_zone == 2 && dock_index > 0) dock_index--;
                        break;
                    case uli::runtime::InputInterpreter::Key::RIGHT:
                        if (focus_zone == 2 && dock_index < 1) dock_index++;
                        break;
                    case uli::runtime::InputInterpreter::Key::TAB:
                        focus_zone = (focus_zone + 1) % 3;
                        break;
                    case uli::runtime::InputInterpreter::Key::SPACE:
                    case uli::runtime::InputInterpreter::Key::ENTER:
                        if (focus_zone == 1) { 
                            if (!search_results.empty() && result_index < static_cast<int>(search_results.size())) {
                                selected_tz = search_results[result_index];
                                running = false; // Picking from table immediately saves
                            }
                        } else if (focus_zone == 2) {
                            if (dock_index == 0) { // Save
                                running = false;
                            } else if (dock_index == 1) { // Cancel
                                selected_tz = current_tz; // revert
                                running = false;
                            }
                        }
                        break;
                    case uli::runtime::InputInterpreter::Key::ESC:
                        selected_tz = current_tz;
                        running = false;
                        break;
                    default:
                        break;
                }
            }
        }

        std::cout << "\033[?25h";
        uli::runtime::InputInterpreter::disable_raw_mode();
        return selected_tz;
    }

    static void run_setup_flow(uli::runtime::MenuState& state) {
        bool in_menu = true;
        while (in_menu) {
            std::vector<std::string> opts = {
                "Set Timezone (Current: " + state.timezone + ")",
                "Hardware Clock Format (Current: " + std::string(state.rtc_in_utc ? "UTC" : "Local Time") + ")",
                "Network Time Sync (Current: " + std::string(state.ntp_sync ? "ON" : "OFF") + ")",
                "Back"
            };

            int sel = uli::runtime::DialogBox::ask_selection("Time & Date Configuration", "Select an option to configure:", opts);

            if (sel == 0) {
                // Timezone search UI
                std::string chosen_tz = run_live_search(state.timezone);
                if (!chosen_tz.empty()) {
                    state.timezone = chosen_tz;
                }
                uli::runtime::DesignUI::clear_screen();
            } else if (sel == 1) {
                // Hardware Clock configuration (RTC)
                std::vector<std::string> rtc_opts = {
                    "UTC (Recommended for Linux/macOS)",
                    "Local Time (Compatible with Windows Dual-Boot)"
                };
                int rtc_sel = uli::runtime::DialogBox::ask_selection("Hardware Clock", "How should the hardware clock (RTC) be managed?", rtc_opts);
                if (rtc_sel == 0) {
                    state.rtc_in_utc = true;
                } else if (rtc_sel == 1) {
                    state.rtc_in_utc = false;
                }
            } else if (sel == 2) {
                run_ntp_setup(state);
            } else {
                in_menu = false;
            }
        }
    }

    static void run_ntp_setup(uli::runtime::MenuState& state) {
        // NTP Configuration
        bool use_ntp = uli::runtime::DialogBox::ask_yes_no("Network Time Protocol", "Enable automated network time synchronization (NTP)?");
        state.ntp_sync = use_ntp;

        if (use_ntp) {
            std::string custom_server = uli::runtime::DialogBox::ask_input("NTP Server", "Enter the target NTP server endpoint (default: time.google.com):");
            if (!custom_server.empty()) {
                state.ntp_server = custom_server;
            } else {
                state.ntp_server = "time.google.com"; // Fallback to safe default
            }
        }
    }
};

} // namespace timedate
} // namespace uli

#endif // ULI_TIMEDATE_UI_TZ_SELECT_HPP
