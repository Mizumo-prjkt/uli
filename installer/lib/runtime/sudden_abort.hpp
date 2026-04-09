#ifndef ULI_RUNTIME_SUDDEN_ABORT_HPP
#define ULI_RUNTIME_SUDDEN_ABORT_HPP

#include <csignal>
#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <termios.h>
#include "blackbox.hpp"

namespace uli {
namespace runtime {

class SuddenAbort {
public:
    static void restore_terminal() {
        // Exit alternate screen buffer
        std::cout << "\033[?1049l";
        // Show cursor
        std::cout << "\033[?25h";
        // Reset text formatting
        std::cout << "\033[0m\n";
        std::cout.flush();

        // Restore terminal canonical mode and echoing safely
        struct termios ts;
        if (tcgetattr(STDIN_FILENO, &ts) != -1) {
            ts.c_lflag |= (ECHO | ICANON);
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &ts);
        }
    }

    static void signal_handler(int signum) {
        BlackBox::log("CRASH DETECTED: Received signal " + std::to_string(signum) + " (" + strsignal(signum) + ")");
        restore_terminal();
        std::cerr << "\n[ULI] Terminal session aborted. Received signal: " << signum << " (" << strsignal(signum) << ")\n";
        
        // Use standard abort sequence for correct return status
        std::_Exit(128 + signum);
    }

    static void register_handlers() {
        // Intercept standard terminal interrupts and failures
        std::signal(SIGINT, signal_handler);   // Ctrl+C
        std::signal(SIGSEGV, signal_handler);  // Segmentation fault
        std::signal(SIGABRT, signal_handler);  // Abort
        std::signal(SIGTERM, signal_handler);  // Termination request
        std::signal(SIGQUIT, signal_handler);  // Quit from keyboard
        std::signal(SIGILL, signal_handler);   // Illegal Instruction
        std::signal(SIGFPE, signal_handler);   // Floating point exception
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_SUDDEN_ABORT_HPP
