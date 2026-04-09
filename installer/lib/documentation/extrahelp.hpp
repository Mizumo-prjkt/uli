#ifndef ULI_EXTRAHELP_HPP
#define ULI_EXTRAHELP_HPP

#include <iostream>

namespace uli {
namespace documentation {

inline void print_extra_help() {
    std::cout << "Universal Linux Installer (ULI) - Extended Help\n"
              << "===============================================\n\n"
              << "Deployment Profiles:\n"
              << "  A deployment profile is a YAML file that defines the desired state\n"
              << "  of the target system. It includes configuration for:\n"
              << "    - System: hostname, timezone, locale\n"
              << "    - Boot: kernel args, plymouth themes\n"
              << "    - Storage: partitioning schemes (efi, swap, root)\n"
              << "    - Users: username, password, groups\n"
              << "    - Software: abstract packages to install\n"
              << "    - Services: abstract services to enable/start\n\n"
              << "Translation Dictionaries:\n"
              << "  A translation dictionary is a YAML file used by the engine to map\n"
              << "  the abstract configurations in a profile to a specific Linux distribution.\n"
              << "  It defines:\n"
              << "    - package_manager: binary name, install/remove commands\n"
              << "    - package_map: abstract pkg -> native pkg(s) (e.g. nginx -> nginx)\n"
              << "    - service_map: abstract service -> native systemd name\n\n"
              << "Architecture:\n"
              << "  ULI reads the profile and dictionary, translates the abstract packages\n"
              << "  and services, and constructs the native commands for execution.\n\n"
              << "For standard usage, run `uli_installer --help`.\n";
}

} // namespace documentation
} // namespace uli

#endif // ULI_EXTRAHELP_HPP
