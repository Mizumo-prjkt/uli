#ifndef ULI_RUNTIME_INPUTBOX_HPP
#define ULI_RUNTIME_INPUTBOX_HPP

#include "design_ui.hpp"
#include "input_interpreter.hpp"
#include "ui_menudesign.hpp"
#include "test_simulation.hpp"
#include <iostream>
#include <string>

namespace uli {
namespace runtime {

class InputBox {
public:
  // Captures raw text input while rendering a box
  static std::string ask_input(const std::string &title,
                               const std::string &prompt,
                               std::string default_val = "",
                               size_t max_len = 256,
                               bool is_password = false) {
    std::string buffer = default_val;
    size_t cursor_pos = buffer.length();
    std::string error_msg = "";

    InputInterpreter::enable_raw_mode();
    std::cout << "\033[2J\033[H"; // Initial clear is fine
    std::cout << "\033[?25h";

    bool running = true;
    while (running) {
      std::cout << "\033[H\033[J"; // Move home and clear down
      DesignUI::print_header(title);
      std::cout << "\n  " << prompt << "\n\n";

      // Draw an aesthetic input field (dynamic sizing based on content)
      size_t box_width = std::max((size_t)40, std::max(buffer.length(), default_val.length()) + 2);
      std::cout << "  " << DesignUI::DIM << "+" << std::string(box_width, '-')
                << "+\n";
      
      std::string pt1 = buffer.substr(0, cursor_pos);
      std::string pt2 = buffer.substr(cursor_pos);
      
      if (is_password && !uli::runtime::TestSimulation::get_config().no_masking) {
        pt1 = std::string(pt1.length(), '*');
        pt2 = std::string(pt2.length(), '*');
      }
      
      std::cout << "  | " << DesignUI::RESET << pt1 << "\033[s" << pt2
                << DesignUI::DIM << std::string(box_width - 1 - buffer.length(), ' ')
                << "|\n";
      std::cout << "  +" << std::string(box_width, '-') << "+" << DesignUI::RESET
                << "\n";

      if (!error_msg.empty()) {
        std::cout << "  \033[31m" << error_msg << "\033[0m\n\n";
      } else {
        std::cout << "\n\n";
      }

      std::cout << "  (Press ENTER to confirm, ESC to cancel)\n";

      // Move cursor to visually type inside the box block using ANSI save and
      // restore
      std::cout << "\033[u";
      std::cout << std::flush;

      char ch = 0;
      InputInterpreter::Key k = InputInterpreter::read_key(ch, true);

      error_msg = ""; // Clear warning upon new keystroke

      if (k == InputInterpreter::Key::ENTER) {
        running = false;
      } else if (k == InputInterpreter::Key::LEFT) {
        if (cursor_pos > 0) cursor_pos--;
      } else if (k == InputInterpreter::Key::RIGHT) {
        if (cursor_pos < buffer.length()) cursor_pos++;
      } else if (k == InputInterpreter::Key::BACKSPACE) {
        if (cursor_pos > 0) {
          buffer.erase(cursor_pos - 1, 1);
          cursor_pos--;
        }
      } else if (k == InputInterpreter::Key::ESC) {
        buffer = "";
        running = false;
      } else if (k == InputInterpreter::Key::OTHER) {
        if (ch >= 32 && ch <= 126) {
          // Block common bash injection tokens to prevent script escaping
          if (ch == ';' || ch == '|' || ch == '&' || ch == '$' || ch == '>' ||
              ch == '<' || ch == '\\' || ch == '`' || ch == '\'' || ch == '"' ||
              ch == '^') {
            error_msg = "Warning: Shell-sensitive characters are disallowed!";
          } else if (buffer.length() >= max_len) {
            error_msg = "Warning: " + std::to_string(max_len) + " characters maximum limit reached.";
          } else {
            buffer.insert(cursor_pos, 1, ch);
            cursor_pos++;
          }
        }
      }
    }

    InputInterpreter::disable_raw_mode();
    std::cout << "\033[2J\x1B[H";
    return buffer;
  }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_INPUTBOX_HPP
