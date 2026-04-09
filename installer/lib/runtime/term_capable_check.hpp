#ifndef ULI_RUNTIME_TERM_CAPABLE_CHECK_HPP
#define ULI_RUNTIME_TERM_CAPABLE_CHECK_HPP

#include <string>
#include <cstdlib>
#include <sys/ioctl.h>
#include <unistd.h>
#include <csignal>
#include <iostream>

namespace uli {
namespace runtime {

class TermCapableCheck {
public:
    // Determine if the terminal environment claims color capability
    static bool supports_color() {
        const char* term = std::getenv("TERM");
        if (!term) return false;
        
        std::string t(term);
        // Common terminal identifiers that support color output
        return t.find("color") != std::string::npos || 
               t.find("xterm") != std::string::npos ||
               t.find("alacritty") != std::string::npos ||
               t.find("kitty") != std::string::npos ||
               t.find("rxvt") != std::string::npos ||
               t.find("linux") != std::string::npos;
    }

    // Attempt to invoke an IOCTL syscall to harvest terminal dimensions
    static void get_terminal_size(int& cols, int& rows) {
        struct winsize w;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
            cols = w.ws_col;
            rows = w.ws_row;
        } else {
            // Safe fallback defaults
            cols = 80;
            rows = 24;
        }
    }

    // Switch to Alternate Screen Buffer (prevents scrolling, hides history)
    static void enter_alternate_screen() {
        std::cout << "\033[?1049h\033[H"; // smcup and home
        std::cout.flush();
    }

    // Restore standard terminal buffer
    static void exit_alternate_screen() {
        std::cout << "\033[?1049l"; // rmcup
        std::cout.flush();
    }

    // Register a SIGWINCH handler to dynamically respond to console resizing
    static void register_resize_handler(void (*handler)(int)) {
        std::signal(SIGWINCH, handler);
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_TERM_CAPABLE_CHECK_HPP
