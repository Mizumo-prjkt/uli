#ifndef ULI_RUNTIME_UI_DUALPANE_SELECT_HPP
#define ULI_RUNTIME_UI_DUALPANE_SELECT_HPP

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "design_ui.hpp"
#include "input_interpreter.hpp"
#include "dialogbox.hpp"
#include "../packagemanager_layer_interface.hpp"
#include "../package_mgr/linter_pkg.hpp"

namespace uli {
namespace runtime {

class UIDualPaneSelect {
public:
    static std::vector<std::string> show_package_search(
        uli::package_mgr::PackageManagerInterface* pm,
        std::vector<std::string> selected_items = {},
        const std::vector<ManualMapping>& manual_mappings = {}) 
    {
        InputInterpreter::enable_raw_mode();
        std::cout << "\033[?25l";

        bool running = true;
        std::vector<std::string> result = selected_items;
        std::vector<std::string> search_results;
        
        std::string query = "";
        
        int focus_zone = 0; // 0=Search, 1=Result, 2=Selected, 3=Dock
        int result_index = 0;
        int selected_index = 0;
        int dock_index = 0; // 0=Save, 1=Reset, 2=Cancel, 3=Manual Entry

        int visible_height = 8;

        std::string alert_prompt = "";

        while (running) {
            std::cout << "\033[H\033[J";
            DesignUI::print_header("Package Search");
            std::cout << "\n";
            
            if (!alert_prompt.empty()) {
                std::cout << "  \033[31m" << alert_prompt << "\033[0m\n\n";
            }
            
            // Render Zone 0: Search Input
            std::cout << "  Search additional Packages:\n";
            std::cout << "  +--------------------------------+\n";
            if (focus_zone == 0) {
                std::cout << "  + \033[42;30m" << query;
                int pad = 30 - query.length();
                for(int i = 0; i < pad; ++i) std::cout << " ";
                std::cout << "\033[0m +\n";
            } else {
                std::cout << "  + " << query;
                int pad = 30 - query.length();
                for(int i = 0; i < pad; ++i) std::cout << " ";
                std::cout << " +\n";
            }
            std::cout << "  +--------------------------------+\n\n";

            // Render Zone 1 & 2: Tables
            std::string l_head = "| Result                       |";
            std::string l_sep  = "|------------------------------|";
            
            std::string r_head = "| Selected                     |";
            std::string r_sep  = "|------------------------------|";

            std::cout << "  " << l_head << "    " << r_head << "\n";
            std::cout << "  " << l_sep  << "    " << r_sep  << "\n";

            int start_res = 0;
            if (result_index >= visible_height) start_res = result_index - visible_height + 1;
            
            int start_sel = 0;
            if (selected_index >= visible_height) start_sel = selected_index - visible_height + 1;

            for (int i = 0; i < visible_height; ++i) {
                int r_idx = start_res + i;
                int s_idx = start_sel + i;
                
                // Left String
                std::string left_str = "|                              |";
                if (r_idx < static_cast<int>(search_results.size())) {
                    std::string opt_text = search_results[r_idx];
                    if (opt_text.length() > 24) opt_text = opt_text.substr(0, 21) + "...";
                    std::string inner = opt_text;
                    while (inner.length() < 28) inner += " ";
                    
                    if (focus_zone == 1 && r_idx == result_index) {
                        left_str = "| \033[42;30m" + inner + "\033[0m |";
                    } else {
                        left_str = "| " + inner + " |";
                    }
                }

                // Middle Divider
                std::string mid = (i == (visible_height / 2)) ? " >> " : "    ";

                // Right String
                std::string right_str = "|                              |";
                if (s_idx < static_cast<int>(result.size())) {
                    std::string sel_text = result[s_idx];
                    if (sel_text.length() > 24) sel_text = sel_text.substr(0, 21) + "...";
                    std::string inner = sel_text;
                    while (inner.length() < 28) inner += " ";
                    
                    if (focus_zone == 2 && s_idx == selected_index) {
                        right_str = "| \033[42;30m" + inner + "\033[0m |";
                    } else {
                        right_str = "| " + inner + " |";
                    }
                }
                
                std::cout << "  " << left_str << mid << right_str << "\n";
            }
            std::cout << "  " << l_sep  << "    " << r_sep  << "\n\n";

            // Render Zone 3: Dock
            std::vector<std::string> dock_options = {"Save", "Reset", "Cancel", "Enter Manual entry to Selected"};
            std::cout << "  ";
            for (size_t i = 0; i < dock_options.size(); ++i) {
                if (focus_zone == 3 && static_cast<int>(i) == dock_index) {
                    std::cout << "\033[42;30m[" << dock_options[i] << "]\033[0m ";
                } else {
                    std::cout << "[" << dock_options[i] << "] ";
                }
            }
            std::cout << "\n\n  \033[90mPress TAB key to navigate through the UI\033[0m\n";
            
            // Input Mode logic
            if (focus_zone == 0) {
                char c = 0;
                InputInterpreter::Key k = InputInterpreter::read_key(c, true); // true = text Mode bypass
                if (c == '\t') {
                    focus_zone = 1;
                } else if (k == InputInterpreter::Key::ENTER) {
                    // Re-run search query explicitly if desired, but we do live search
                } else if (k == InputInterpreter::Key::BACKSPACE) {
                    if (!query.empty()) {
                        query.pop_back();
                        search_results = pm->search_packages(query);
                        if (!search_results.empty() && search_results[0] == "_ULI_PM_ERROR_") {
                            search_results.clear();
                        }
                    } else {
                        search_results.clear();
                    }
                    result_index = 0;
                } else if (k == InputInterpreter::Key::ESC) {
                    // Do nothing or map to cancel
                } else if (k == InputInterpreter::Key::OTHER) {
                    if (c >= 32 && c <= 126 && query.length() < 30) {
                        query += c;
                        search_results = pm->search_packages(query);
                        if (!search_results.empty() && search_results[0] == "_ULI_PM_ERROR_") {
                            search_results.clear();
                        }
                        result_index = 0;
                    }
                }
            } else {
                InputInterpreter::Key k = InputInterpreter::read_key();
                switch (k) {
                    case InputInterpreter::Key::UP:
                        if (focus_zone == 1 && result_index > 0) result_index--;
                        else if (focus_zone == 2 && selected_index > 0) selected_index--;
                        break;
                    case InputInterpreter::Key::DOWN:
                        if (focus_zone == 1 && result_index < static_cast<int>(search_results.size()) - 1) result_index++;
                        else if (focus_zone == 2 && selected_index < static_cast<int>(result.size()) - 1) selected_index++;
                        break;
                    case InputInterpreter::Key::LEFT:
                        if (focus_zone == 3 && dock_index > 0) dock_index--;
                        break;
                    case InputInterpreter::Key::RIGHT:
                        if (focus_zone == 3 && dock_index < 3) dock_index++;
                        break;
                    case InputInterpreter::Key::TAB:
                        focus_zone = (focus_zone + 1) % 4;
                        break;
                    case InputInterpreter::Key::SPACE:
                    case InputInterpreter::Key::ENTER:
                        if (focus_zone == 1) { // Add from result to selected
                            if (!search_results.empty() && result_index < static_cast<int>(search_results.size())) {
                                std::string pkg = search_results[result_index];
                                if (std::find(result.begin(), result.end(), pkg) == result.end()) {
                                    result.push_back(pkg);
                                }
                            }
                        } else if (focus_zone == 2) { // Remove from selected
                            if (!result.empty() && selected_index < static_cast<int>(result.size())) {
                                result.erase(result.begin() + selected_index);
                                if (selected_index >= static_cast<int>(result.size()) && selected_index > 0) {
                                    selected_index--;
                                }
                            }
                        } else if (focus_zone == 3) {
                            if (dock_index == 0) { // Save
                                running = false;
                            } else if (dock_index == 1) { // Reset
                                result.clear();
                                selected_index = 0;
                            } else if (dock_index == 2) { // Cancel
                                result = selected_items;
                                running = false;
                            } else if (dock_index == 3) { // Manual Entry
                                InputInterpreter::disable_raw_mode();
                                std::cout << "\033[?25h";
                                std::cout << "\033[H\033[J"; // Clear before DialogBox
                                std::string man_ent = DialogBox::ask_input("Manual Entry", "Enter direct package name explicitly.\n\nWARNING: Ensure strict spelling or else the install may fail at runtime!");
                                if (!man_ent.empty() && std::find(result.begin(), result.end(), man_ent) == result.end()) {
                                    // Check if this matches a Manual Mapping generic name
                                    bool is_generic_mapped = false;
                                    for (const auto& m : manual_mappings) {
                                        if (m.generic_name == man_ent) {
                                            is_generic_mapped = true;
                                            break;
                                        }
                                    }

                                    if (is_generic_mapped) {
                                        DialogBox::show_alert("Generic Component Detected", "The name '" + man_ent + "' matches a manual mapping generic name. It will be added as a linked component.");
                                        result.push_back(man_ent);
                                    } else {
                                        auto lint_res = uli::package_mgr::PackageLinter::verify_package(pm, man_ent);
                                        if (lint_res == uli::package_mgr::LintResult::NOT_FOUND) {
                                            DialogBox::show_alert("Package Not Found", "The package '" + man_ent + "' does not exist in the repository indexes! Installation prevented.");
                                        } else {
                                            if (lint_res == uli::package_mgr::LintResult::PM_UNAVAILABLE) {
                                                DialogBox::show_alert("Linter Offline", "The package manager is unresponsive or lacks network. The engine cannot verify this package safely. You are on your own.");
                                            }
                                            result.push_back(man_ent);
                                        }
                                    }
                                }
                                InputInterpreter::enable_raw_mode();
                                std::cout << "\033[?25l";
                            }
                        }
                        break;
                    case InputInterpreter::Key::BACKSPACE:
                        if (focus_zone == 2) { // Remove from selected
                            if (!result.empty() && selected_index < static_cast<int>(result.size())) {
                                result.erase(result.begin() + selected_index);
                                if (selected_index >= static_cast<int>(result.size()) && selected_index > 0) {
                                    selected_index--;
                                }
                            }
                        }
                        break;
                    case InputInterpreter::Key::ESC:
                        result = selected_items;
                        running = false;
                        break;
                    default:
                        break;
                }
            }
        }

        std::cout << "\033[?25h";
        InputInterpreter::disable_raw_mode();
        return result;
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_UI_DUALPANE_SELECT_HPP
