#ifndef ULI_RUNTIME_PROCESS_SUPERVISOR_HPP
#define ULI_RUNTIME_PROCESS_SUPERVISOR_HPP

#include <iostream>
#include <string>
#include <vector>
#include <termios.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <signal.h>
#include "warn.hpp"
#include "blackbox.hpp"
#include "../documentation/extrahelp.hpp"

namespace uli {
namespace runtime {

enum class SupervisionResult {
    SUCCESS,
    FAILURE,
    RESTART_SOFT, // Ctrl+U or O->o
    RESTART_HARD, // Ctrl+R or O->Enter
    ABORT         // Ctrl+C
};

class ProcessSupervisor {
public:
    static SupervisionResult execute(const std::string& command) {
        BlackBox::log("SUPERVISOR: Executing " + command);
        
        pid_t child_pid = fork();
        if (child_pid == 0) {
            // Child process: set process group to itself to allow killing sub-tree
            setpgid(0, 0); 
            execl("/bin/sh", "sh", "-c", command.c_str(), (char*)NULL);
            exit(1);
        } else if (child_pid < 0) {
            Warn::print_error("Failed to fork supervisor child.");
            return SupervisionResult::FAILURE;
        }

        // Parent process
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        SupervisionResult result = SupervisionResult::SUCCESS;
        int status;

        while (true) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(STDIN_FILENO, &read_fds);

            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000; // 100ms polling

            int sret = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &tv);

            // Check if child finished
            pid_t wpid = waitpid(child_pid, &status, WNOHANG);
            if (wpid == child_pid) {
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    result = SupervisionResult::SUCCESS;
                } else {
                    result = SupervisionResult::FAILURE;
                }
                break;
            } else if (wpid < 0) {
                result = SupervisionResult::FAILURE;
                break;
            }

            if (sret > 0 && FD_ISSET(STDIN_FILENO, &read_fds)) {
                char c;
                if (read(STDIN_FILENO, &c, 1) > 0) {
                    if (c == 0x15) { // Ctrl+U (NAK)
                        BlackBox::log("SUPERVISOR: Caught Ctrl+U (Soft Restart)");
                        kill(-child_pid, SIGKILL); // Kill process group
                        waitpid(child_pid, &status, 0);
                        result = SupervisionResult::RESTART_SOFT;
                        break;
                    } else if (c == 0x12) { // Ctrl+R (DC2)
                        BlackBox::log("SUPERVISOR: Caught Ctrl+R (Hard Restart)");
                        kill(-child_pid, SIGKILL); // Kill process group
                        waitpid(child_pid, &status, 0);
                        result = SupervisionResult::RESTART_HARD;
                        break;
                    } else if (c == 0x08) { // Ctrl+H (BS/Help)
                        uli::documentation::print_supervision_help();
                        std::cout << "\n[INFO] Resuming session monitoring...\n";
                    } else if (c == 0x0F) { // Ctrl+O (SI)
                        BlackBox::log("SUPERVISOR: Caught Ctrl+O (Hold Mode)");
                        kill(-child_pid, SIGKILL); // Kill process group
                        waitpid(child_pid, &status, 0);
                        
                        std::cout << "\n\n\033[1;33m--- INSTALLER ON HOLD ---\033[0m\n";
                        std::cout << "Installation paused. Select your next step:\n";
                        std::cout << " [o] Restart package installation (Soft Retry)\n";
                        std::cout << " [Enter] Restart from drive formatting (Hard Redo)\n";
                        std::cout << " [h] View Troubleshooting Help\n";
                        std::cout << " [Any other key] Cancel and Abort\n";
                        
                        // Wait for specific input in hold mode
                        while (true) {
                            if (read(STDIN_FILENO, &c, 1) > 0) {
                                if (c == 'o' || c == 'O') {
                                    result = SupervisionResult::RESTART_SOFT;
                                    break;
                                } else if (c == '\n' || c == '\r') {
                                    result = SupervisionResult::RESTART_HARD;
                                    break;
                                } else if (c == 'h' || c == 'H') {
                                    uli::documentation::print_supervision_help();
                                    std::cout << "\n[HOLD] Select o, Enter, or any other key to abort: ";
                                    continue;
                                } else {
                                    result = SupervisionResult::FAILURE;
                                    break;
                                }
                            }
                        }
                        break;
                    } else if (c == 0x03) { // Ctrl+C (ETX/Abort)
                        BlackBox::log("SUPERVISOR: Caught Ctrl+C (Abort Engine)");
                        kill(-child_pid, SIGKILL);
                        waitpid(child_pid, &status, 0);
                        result = SupervisionResult::ABORT;
                        break;
                    }
                }
            }
        }

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return result;
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_PROCESS_SUPERVISOR_HPP
