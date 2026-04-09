#ifndef ULI_RUNTIME_DIALOGBOX_HPP
#define ULI_RUNTIME_DIALOGBOX_HPP

#include <iostream>
#include <string>
#include <vector>
#include "design_ui.hpp"
#include "warn.hpp"
#include "ui_menudesign.hpp"
#include "inputbox.hpp"
#include <functional>

namespace uli {
namespace runtime {

class DialogBox {
public:
    // Renders a message and waits for a [y/N] user response
    static bool ask_yes_no(const std::string& title, const std::string& prompt) {
        DesignUI::clear_screen();
        DesignUI::print_header(title);
        std::cout << "\n" << prompt << " [y/N]: ";
        
        std::string input;
        std::getline(std::cin, input);
        
        if (input == "y" || input == "Y" || input == "yes") {
            return true;
        }
        return false;
    }

    // Renders an interactive TUI list of choices and returns the index (0-based) selected 
    static int ask_selection(const std::string& title, const std::string& prompt, const std::vector<std::string>& choices, std::function<std::string()> banner_func = nullptr, const std::string& alert_prompt = "") {
        if (choices.empty()) return -1;
        
        std::string full_title = title;
        if (!prompt.empty()) full_title += " - " + prompt;
        
        return UIMenuDesign::show_interactive_list(full_title, choices, 0, banner_func, alert_prompt);
    }
    
    // Prompts for raw text input utilizing the animated TUI box
    static std::string ask_input(const std::string& title, const std::string& prompt, std::string default_val = "", size_t max_len = 256) {
        return InputBox::ask_input(title, prompt, default_val, max_len, false);
    }

    // Prompts for raw text input visually masked with asterisks
    static std::string ask_password(const std::string& title, const std::string& prompt, size_t max_len = 256) {
        return InputBox::ask_input(title, prompt, "", max_len, true);
    }

    // Displays a simple alert message with an "OK" confirmation button
    static void show_alert(const std::string& title, const std::string& message, std::function<std::string()> banner_func = nullptr) {
        UIMenuDesign::show_interactive_list(title, {"OK"}, 0, banner_func, message);
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_DIALOGBOX_HPP
