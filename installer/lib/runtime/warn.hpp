#ifndef ULI_RUNTIME_WARN_HPP
#define ULI_RUNTIME_WARN_HPP

#include <iostream>
#include <string>
#include "design_ui.hpp"
#include "beep.hpp"

namespace uli {
namespace runtime {

class Warn {
public:
    static void print_warning(const std::string& msg) {
        Beep::play_alert();
        std::cout << DesignUI::YELLOW << DesignUI::BOLD << "[WARNING] " << DesignUI::RESET
                  << DesignUI::YELLOW << msg << DesignUI::RESET << std::endl;
    }

    static void print_error(const std::string& msg) {
        Beep::play_alert();
        std::cout << DesignUI::RED << DesignUI::BOLD << "[CRITICAL ERROR] " << DesignUI::RESET
                  << DesignUI::RED << msg << DesignUI::RESET << std::endl;
    }
    
    static void print_info(const std::string& msg) {
        std::cout << DesignUI::GREEN << DesignUI::BOLD << "[INFO] " << DesignUI::RESET
                  << msg << std::endl;
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_WARN_HPP
