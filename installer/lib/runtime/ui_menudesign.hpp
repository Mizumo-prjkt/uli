#ifndef ULI_RUNTIME_UI_MENUDESIGN_HPP
#define ULI_RUNTIME_UI_MENUDESIGN_HPP

#include <iostream>
#include <string>
#include <vector>
#include "design_ui.hpp"
#include "input_interpreter.hpp"
#include <functional>

namespace uli {
namespace runtime {

class UIMenuDesign {
public:
    // Displays an interactive bounded dialog array capturing arrow keys mapping to indices
    static int show_interactive_list(const std::string& title, const std::vector<std::string>& options, int default_index = 0, std::function<std::string()> banner_func = nullptr, const std::string& alert_prompt = "") {
        if (options.empty()) return -1;

        int selected = default_index;
        if (selected < 0 || selected >= static_cast<int>(options.size())) {
            selected = 0;
        }

        InputInterpreter::enable_raw_mode();
        std::cout << "\033[?25l";

        bool running = true;
        while (running) {
            // Draw visual prompt bounding
            std::cout << "\033[H\033[J"; // Move home and clear down (avoids flicker)
            DesignUI::print_header(title);
            std::cout << "\n";
            std::string banner_text = banner_func ? banner_func() : "";
            if (!banner_text.empty()) {
                std::cout << banner_text << "\n";
            }
            if (!alert_prompt.empty()) {
                std::cout << "\n  \033[31m" << alert_prompt << "\033[0m\n\n";
            }

            for (size_t i = 0; i < options.size(); ++i) {
                if (static_cast<int>(i) == selected) {
                    // Highlight selected row with Green background, Black text
                    std::cout << "  \033[42;30m " << options[i] << " \033[0m\n";
                } else {
                    std::cout << "    " << options[i] << " \n";
                }
            }

            InputInterpreter::Key k = InputInterpreter::read_key();
            switch (k) {
                case InputInterpreter::Key::UP:
                    if (selected > 0) selected--;
                    break;
                case InputInterpreter::Key::DOWN:
                    if (selected < static_cast<int>(options.size()) - 1) selected++;
                    break;
                case InputInterpreter::Key::ENTER:
                    running = false;
                    break;
                case InputInterpreter::Key::ESC:
                case InputInterpreter::Key::BACKSPACE:
                    selected = -1; // Indicate cancellation
                    running = false;
                    break;
                default:
                    break;
            }
        }

        std::cout << "\033[?25h";
        InputInterpreter::disable_raw_mode();
        
        return selected;
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_UI_MENUDESIGN_HPP
