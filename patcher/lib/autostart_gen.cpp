#include "autostart_gen.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace uli {
namespace patcher {

std::string AutostartGen::generate_script() {
    std::string script;

    script += "#!/bin/bash\n";
    script += "# =============================================\n";
    script += "# ULI Autostart Hook — Injected by uli_patcher\n";
    script += "# =============================================\n";
    script += "# This script is invoked when the ULI boot\n";
    script += "# menu entry is selected. It checks for the\n";
    script += "# uli_autostart kernel parameter and launches\n";
    script += "# the installer automatically.\n";
    script += "#\n";
    script += "# It can also be executed manually from the\n";
    script += "# live environment shell:\n";
    script += "#   chmod +x /uli/autostart.sh && /uli/autostart.sh\n";
    script += "# =============================================\n\n";

    // Wait for the system to finish initializing
    script += "sleep 2\n\n";

    // Check for kernel parameter
    script += "if grep -q 'uli_autostart=1' /proc/cmdline 2>/dev/null; then\n";
    script += "    echo '[ULI] Autostart flag detected. Launching ULI Installer...'\n";
    script += "    \n";
    script += "    # Ensure the installer binary is executable\n";
    script += "    chmod +x /uli/uli_installer 2>/dev/null\n";
    script += "    \n";
    script += "    # Check if profile exists\n";
    script += "    if [ -f /uli/profile.yaml ]; then\n";
    script += "        echo '[ULI] Loading profile: /uli/profile.yaml'\n";
    script += "        /uli/uli_installer --profile /uli/profile.yaml\n";
    script += "    else\n";
    script += "        echo '[ULI] No profile found, launching interactive mode.'\n";
    script += "        /uli/uli_installer\n";
    script += "    fi\n";
    script += "else\n";
    script += "    echo '[ULI] No autostart flag detected. Run manually:'\n";
    script += "    echo '  /uli/autostart.sh'\n";
    script += "fi\n";

    return script;
}

std::string AutostartGen::write_to_temp(const std::string& temp_dir) {
    std::string path = temp_dir + "/autostart.sh";
    fs::create_directories(temp_dir);

    std::ofstream f(path);
    if (!f.is_open()) {
        std::cerr << "[AutostartGen] Failed to write: " << path << std::endl;
        return "";
    }

    f << generate_script();
    f.close();

    // Make executable
    fs::permissions(path, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                    fs::perm_options::add);

    std::cout << "[AutostartGen] Generated autostart script: " << path << std::endl;
    return path;
}

} // namespace patcher
} // namespace uli
