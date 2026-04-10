#ifndef ULI_HELPGUIDE_HPP
#define ULI_HELPGUIDE_HPP

#include <iostream>

namespace uli {
namespace documentation {

inline void print_help_guide() {
  std::cout
      << "Universal Linux Installer (ULI) - Help Guide\n"
      << "============================================\n"
      << "Usage:\n"
      << "  uli_installer [OPTIONS] [COMMANDS]\n\n"
      << "Description:\n"
      << "  ULI is a unified tool designed to deploy and standardise\n"
      << "  Linux installations across multiple distributions (Debian, Arch, "
         "Alpine).\n"
      << "  It reads a standard YAML deployment profile and uses translation\n"
      << "  dictionaries to configure the system with native package "
         "managers.\n\n"
      << "Commands:\n"
      << "  install       Starts the installation process given a profile.\n"
      << "  translate     Tests the package translation against a "
         "dictionary.\n\n"
      << "Options:\n"
      << "  --help, -h    Displays this standard help guide.\n"
      << "  --extra-help  Displays extended documentation for profiles and "
         "translation.\n"
      << "  --profile     Specify the path to the YAML deployment profile.\n"
      << "  --lintcheck   Validates the YAML profile for syntax and logic "
         "errors.\n"
      << "  --disable-builtin-dict Disables the onboard instruction fallback "
         "(embedded trans.yaml).\n"
      << "  --dict        Specify the path to the YAML package translation "
         "dictionary.\n"
      << "                (Maps abstract package names to distro-native "
         "equivalents.\n"
      << "                 NOT for language translations. See --extra-help for "
         "details.)\n"
      << "  --force-small-disk  Forces installation to proceed on disks under "
         "14 GiB.\n"
      << "                (\033[1;31mNOT RECOMMENDED\033[0m: Automator scaling "
         "will be unpredictable)\n"
      << "  --unattended  Enables non-interactive, automatic processing of the "
         "configuration.\n"
      << "  --load-last-error Resumes the installation from the last saved "
         "checkpoint in /tmp.\n\n"
#ifdef ULI_DEBUG_MODE
      << "Debug Options:\n"
      << "  --debug-mode  Enables test simulation mode.\n"
      << "  --dist        Specify mock distribution. Supports versioning:\n"
      << "                arch, debian, debian:12, debian:13, alpine\n"
      << "  --disk        Specify mock disk size (e.g. 500G).\n"
      << "  --disk-type   Specify mock disk type (e.g. nvme, sata).\n"
      << "  --hardware    Specify mock hardware environment.\n"
      << "  --test-memleak Starts a memory leak diagnostic loop.\n"
      << "  --no-masking  Disables masking in test simulation.\n\n"
#endif
      << "Examples:\n"
      << "  uli_installer --help\n"
      << "  uli_installer install --profile deploy.yaml\n";
}

} // namespace documentation
} // namespace uli

#endif // ULI_HELPGUIDE_HPP
