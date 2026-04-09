#ifndef ULI_RUNTIME_UI_MULTISELECT_HPP
#define ULI_RUNTIME_UI_MULTISELECT_HPP

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "design_ui.hpp"
#include "input_interpreter.hpp"
#include <functional>

namespace uli {
namespace runtime {

class UIMultiSelectDesign {
public:
    static std::vector<std::string> show_multi_selection_list(
        const std::string& title, 
        const std::vector<std::string>& options, 
        std::vector<std::string> selected_items = {}, 
        std::function<std::string()> banner_func = nullptr, 
        const std::string& alert_prompt = "") 
    {
        if (options.empty()) return selected_items;

        // Create a boolean map of selected items
        std::vector<bool> is_selected(options.size(), false);
        for (const auto& sel : selected_items) {
            auto it = std::find(options.begin(), options.end(), sel);
            if (it != options.end()) {
                is_selected[std::distance(options.begin(), it)] = true;
            }
        }

        int list_index = 0;
        int dock_index = 0; // 0 = Save, 1 = Reset, 2 = Back
        bool dock_focused = false; // false = List, true = Dock

        InputInterpreter::enable_raw_mode();
        std::cout << "\033[?25l";

        bool running = true;
        std::vector<std::string> result = selected_items;

        int visible_height = 10;
        int max_option_width = 30;
        int max_selected_width = 30;

        while (running) {
            std::cout << "\033[H\033[J";
            DesignUI::print_header(title);
            std::cout << "\n";
            std::string banner_text = banner_func ? banner_func() : "";
            if (!banner_text.empty()) {
                std::cout << banner_text << "\n";
            }
            if (!alert_prompt.empty()) {
                std::cout << "\n  \033[31m" << alert_prompt << "\033[0m\n\n";
            }

            // Build Left Pane (Available)
            std::string l_head = "| Mirrors Available           |";
            std::string l_sep  = "|-----------------------------|";
            
            // Build Right Pane (Selected)
            std::string r_head = "| Selected                    |";
            std::string r_sep  = "|-----------------------------|";

            std::cout << "\n  " << l_head << "      " << r_head << "\n";
            std::cout << "  " << l_sep  << "      " << r_sep  << "\n";

            // Figure out scrolling
            int start_idx = 0;
            if (list_index >= visible_height) {
                start_idx = list_index - visible_height + 1;
            }

            // Figure out currently selected items for right pane
            std::vector<std::string> current_selections;
            for (size_t i = 0; i < options.size(); ++i) {
                if (is_selected[i]) current_selections.push_back(options[i]);
            }

            for (int i = 0; i < visible_height; ++i) {
                int opt_idx = start_idx + i;
                
                // Left String
                std::string left_str = "|                             |";
                if (opt_idx < static_cast<int>(options.size())) {
                    std::string prefix = is_selected[opt_idx] ? "[x] " : "[ ] ";
                    std::string opt_text = options[opt_idx];
                    if (opt_text.length() > 22) opt_text = opt_text.substr(0, 19) + "...";
                    
                    std::string inner = prefix + opt_text;
                    // padding to 27 chars
                    while (inner.length() < 27) inner += " ";
                    
                    if (!dock_focused && opt_idx == list_index) {
                        left_str = "| \033[42;30m" + inner + "\033[0m |";
                    } else {
                        left_str = "| " + inner + " |";
                    }
                }

                // Middle Divider
                std::string mid = (i == (visible_height / 2)) ? "  >>  " : "      ";

                // Right String
                std::string right_str = "|                             |";
                if (i < static_cast<int>(current_selections.size())) {
                    std::string sel_text = current_selections[i];
                    if (sel_text.length() > 27) sel_text = sel_text.substr(0, 24) + "...";
                    while (sel_text.length() < 27) sel_text += " ";
                    right_str = "| " + sel_text + " |";
                }
                
                std::cout << "  " << left_str << mid << right_str << "\n";
            }
            
            std::cout << "  " << l_sep  << "      " << r_sep  << "\n\n";

            if (!options.empty() && list_index >= 0 && list_index < static_cast<int>(options.size())) {
                std::cout << "  \033[36mPointer Selected:\033[0m " << options[list_index] << "\n\n";
            } else {
                std::cout << "\n\n";
            }

            // Print Dock
            std::vector<std::string> dock_options = {"Save", "Reset", "Back"};
            std::cout << "  ";
            for (size_t i = 0; i < dock_options.size(); ++i) {
                if (dock_focused && static_cast<int>(i) == dock_index) {
                    std::cout << "\033[42;30m[" << dock_options[i] << "]\033[0m ";
                } else {
                    std::cout << "[" << dock_options[i] << "] ";
                }
            }
            std::cout << "\n\n  \033[90m<Space> toggle | <Tab> switch focus | <Enter> select\033[0m\n";

            InputInterpreter::Key k = InputInterpreter::read_key();
            switch (k) {
                case InputInterpreter::Key::UP:
                    if (!dock_focused && list_index > 0) list_index--;
                    break;
                case InputInterpreter::Key::DOWN:
                    if (!dock_focused && list_index < static_cast<int>(options.size()) - 1) list_index++;
                    break;
                case InputInterpreter::Key::LEFT:
                    if (dock_focused && dock_index > 0) dock_index--;
                    break;
                case InputInterpreter::Key::RIGHT:
                    if (dock_focused && dock_index < 2) dock_index++;
                    break;
                case InputInterpreter::Key::TAB:
                    dock_focused = !dock_focused;
                    break;
                case InputInterpreter::Key::SPACE:
                case InputInterpreter::Key::ENTER:
                    if (!dock_focused) {
                        is_selected[list_index] = !is_selected[list_index];
                    } else {
                        if (dock_index == 0) { // Save
                            result.clear();
                            for (size_t i = 0; i < options.size(); ++i) {
                                if (is_selected[i]) result.push_back(options[i]);
                            }
                            running = false;
                        } else if (dock_index == 1) { // Reset
                            for (size_t i = 0; i < options.size(); ++i) is_selected[i] = false;
                        } else if (dock_index == 2) { // Back
                            running = false;
                        }
                    }
                    break;
                case InputInterpreter::Key::ESC:
                case InputInterpreter::Key::BACKSPACE:
                    running = false;
                    break;
                default:
                    break;
            }
        }

        std::cout << "\033[?25h";
        InputInterpreter::disable_raw_mode();
        return result;
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_UI_MULTISELECT_HPP
