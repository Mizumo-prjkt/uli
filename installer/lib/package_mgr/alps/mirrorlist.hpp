#ifndef ULI_ALPS_MIRRORLIST_HPP
#define ULI_ALPS_MIRRORLIST_HPP

#include <string>
#include <cstdlib>
#include <iostream>

namespace uli {
namespace package_mgr {
namespace alps {

class Mirrorlist {
public:
    static bool optimize_mirrors() {
        std::cout << "Optimizing Arch Linux mirrors via reflector..." << std::endl;
        std::string cmd = "reflector --verbose -l 200 -n 30 --connection-timeout 1 --cache-timeout 5 --threads 4 --save /etc/pacman.d/mirrorlist --sort rate -p http,https";
        int result = std::system(cmd.c_str());
        return result == 0;
    }
};

} // namespace alps
} // namespace package_mgr
} // namespace uli

#endif // ULI_ALPS_MIRRORLIST_HPP
