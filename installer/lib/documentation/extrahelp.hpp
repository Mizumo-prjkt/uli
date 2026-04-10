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

inline void print_supervision_help() {
    std::cout << "\nInstallation Supervision Controls\n"
              << "===============================\n"
              << "While the installation is running, you can use these shortcuts:\n\n"
              << "  Ctrl + U : Soft Restart\n"
              << "             Terminates the current package installation and restarts it\n"
              << "             immediately. Mounts and partitions are preserved.\n\n"
              << "  Ctrl + R : Hard Restart (Full Nuclear Option)\n"
              << "             Terminates the process, unmounts all partitions, disables swap,\n"
              << "             and restarts everything from disk formatting.\n\n"
              << "  Ctrl + O : Hold Mode\n"
              << "             Pauses the installer and allows you to choose between a\n"
              << "             Soft Restart ('o') or a Hard Restart ('Enter').\n\n"
              << "  Ctrl + H : Help Guide\n"
              << "             Shows this reference manual.\n\n"
              << "  Ctrl + C : Abort Installation\n"
              << "             Terminates everything, unmounts disks, and exits the software.\n\n"
              << "These controls are useful if your internet is slow, a download hangs,\n"
              << "or you realize you need to change your partitioning layout mid-install.\n";
}

} // namespace documentation
} // namespace uli

#endif // ULI_EXTRAHELP_HPP
