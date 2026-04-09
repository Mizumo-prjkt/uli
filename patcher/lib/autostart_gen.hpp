#ifndef ULI_PATCHER_AUTOSTART_GEN_HPP
#define ULI_PATCHER_AUTOSTART_GEN_HPP

#include <string>

namespace uli {
namespace patcher {

class AutostartGen {
public:
    // Generate the autostart shell script content
    // This script runs on boot when uli_autostart=1 is in /proc/cmdline
    static std::string generate_script();

    // Write the autostart script to a temporary path on disk
    // Returns the path to the generated file
    static std::string write_to_temp(const std::string& temp_dir);
};

} // namespace patcher
} // namespace uli

#endif // ULI_PATCHER_AUTOSTART_GEN_HPP
