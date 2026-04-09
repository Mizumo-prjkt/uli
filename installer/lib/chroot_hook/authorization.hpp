#ifndef ULI_AUTHORIZATION_HPP
#define ULI_AUTHORIZATION_HPP

#include <string>
#include <iostream>
#include <unistd.h>
#include <cstdlib>

namespace uli {
namespace hook {

class ChrootHook {
public:
    static bool is_root() {
        return getuid() == 0;
    }

    static int execute_in_chroot(const std::string& target_dir, const std::string& command, const std::string& chroot_cmd = "chroot") {
        if (!is_root()) {
            std::cerr << "Error: Root privileges are required to execute commands in a chroot environment." << std::endl;
            return -1;
        }

        if (target_dir.empty()) {
            std::cerr << "Error: Target chroot directory cannot be empty." << std::endl;
            return -1;
        }

        // Using standard system execution to spawn a chroot shell passing the command
        // Note: For advanced safety and isolation, raw execv/chroot syscall bindings
        // could be evaluated later instead of relying on the host 'chroot' binary.
        std::string full_command = chroot_cmd + " " + target_dir + " /bin/sh -c \"" + command + "\"";
        
        std::cout << "Executing in chroot [" << target_dir << "]: " << command << std::endl;
        int status = std::system(full_command.c_str());

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            std::cerr << "Error: Command did not exit normally in chroot." << std::endl;
            return -1;
        }
    }
};

} // namespace hook
} // namespace uli

#endif // ULI_AUTHORIZATION_HPP
