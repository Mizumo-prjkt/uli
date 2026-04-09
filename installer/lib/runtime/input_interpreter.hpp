#ifndef ULI_RUNTIME_INPUT_INTERPRETER_HPP
#define ULI_RUNTIME_INPUT_INTERPRETER_HPP

#include <termios.h>
#include <unistd.h>
#include <iostream>

namespace uli {
namespace runtime {

class InputInterpreter {
public:
    enum class Key {
        UP,
        DOWN,
        LEFT,
        RIGHT,
        ENTER,
        BACKSPACE,
        ESC,
        RESIZE,
        TAB,
        SPACE,
        OTHER
    };

    // Puts the terminal into raw mode to capture characters instantly
    static void enable_raw_mode() {
        struct termios t;
        tcgetattr(STDIN_FILENO, &t);
        t.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and visual echoing
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
    }

    // Restores terminal back to normal line-buffered mode
    static void disable_raw_mode() {
        struct termios t;
        tcgetattr(STDIN_FILENO, &t);
        t.c_lflag |= (ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
    }

    // Reads a single logical keypress, parsing ANSI escape sequences for arrows
    static Key read_key(char& out_char, bool text_input_mode = false) {
        char ch = 0;
        if (read(STDIN_FILENO, &ch, 1) < 0) {
            if (errno == EINTR) return Key::RESIZE;
            return Key::OTHER;
        }
        out_char = ch;

        if (ch == '\n' || ch == '\r') {
            return Key::ENTER;
        } else if (ch == '\x7F' || ch == '\b') {
            return Key::BACKSPACE;
        } else if (ch == '\x1B') { // Escape sequence detected
            char seq[3];
            if (read(STDIN_FILENO, &seq[0], 1) == 0) return Key::ESC;
            if (read(STDIN_FILENO, &seq[1], 1) == 0) return Key::ESC;

            if (seq[0] == '[') {
                switch (seq[1]) {
                    case 'A': return Key::UP;
                    case 'B': return Key::DOWN;
                    case 'C': return Key::RIGHT;
                    case 'D': return Key::LEFT;
                }
            }
            return Key::ESC;
        } else if (!text_input_mode && ch == '\t') {
            return Key::TAB;
        } else if (!text_input_mode && ch == ' ') {
            return Key::SPACE;
        } else if (!text_input_mode && (ch == 'j' || ch == 'J')) {
            return Key::DOWN;
        } else if (!text_input_mode && (ch == 'k' || ch == 'K')) {
            return Key::UP;
        }
        return Key::OTHER;
    }

    static Key read_key() {
        char dummy;
        return read_key(dummy);
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_INPUT_INTERPRETER_HPP
