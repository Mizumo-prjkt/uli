#ifndef ULI_RUNTIME_KBLAYOUT_HPP
#define ULI_RUNTIME_KBLAYOUT_HPP

#include <string>
#include <vector>

namespace uli {
namespace runtime {
namespace contents {
namespace kb_layout {

class KBLayout {
public:
    // Returns a comprehensive list of standard Linux console/X11 keymap identifiers
    static std::vector<std::string> get_supported_layouts() {
        return {
            "us", "uk", "de", "fr", "es", "it", "pt", "ru", 
            "jp106", "se", "dk", "no", "fi", "nl", "be", 
            "ch", "pl", "cz", "hu", "tr", "br", "latam", 
            "dvorak", "colemak", "ca", "kr", "cn", "in"
        };
    }
};

} // namespace kb_layout
} // namespace contents
} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_KBLAYOUT_HPP
