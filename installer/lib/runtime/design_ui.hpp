#ifndef ULI_RUNTIME_DESIGN_UI_HPP
#define ULI_RUNTIME_DESIGN_UI_HPP

#include <iostream>
#include <string>

namespace uli {
namespace runtime {

class DesignUI {
public:
    // ANSI Escape Codes for standard TUI coloring
    static constexpr const char* RESET   = "\033[0m";
    static constexpr const char* RED     = "\033[31m";
    static constexpr const char* GREEN   = "\033[32m";
    static constexpr const char* YELLOW  = "\033[33m";
    static constexpr const char* BLUE    = "\033[34m";
    static constexpr const char* MAGENTA = "\033[35m";
    static constexpr const char* CYAN    = "\033[36m";
    static constexpr const char* WHITE   = "\033[37m";

    // Text formatting
    static constexpr const char* BOLD    = "\033[1m";
    static constexpr const char* DIM     = "\033[2m";

    // Standard width for bounded dialogs
    static const int DIALOG_WIDTH = 60;

    static void print_border(int width = DIALOG_WIDTH) {
        std::cout << BLUE << std::string(width, '=') << RESET << "\n";
    }

    static void print_header(const std::string& title) {
        int width = DIALOG_WIDTH;
        if (title.length() + 8 > width) {
            width = title.length() + 8;
        }

        print_border(width);
        
        int padding = (width - title.length() - 4) / 2;
        if (padding < 0) padding = 0;
        
        std::cout << BLUE << "||" << RESET;
        std::cout << std::string(padding, ' ');
        std::cout << BOLD << CYAN << title << RESET;
        
        int right_pad = width - padding - title.length() - 4;
        std::cout << std::string(right_pad > 0 ? right_pad : 0, ' ');
        std::cout << BLUE << "||" << RESET << "\n";
        
        print_border(width);
    }
    
    // Clear screen natively avoiding tear flashes
    static void clear_screen() {
        std::cout << "\033[H\033[J";
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_DESIGN_UI_HPP
