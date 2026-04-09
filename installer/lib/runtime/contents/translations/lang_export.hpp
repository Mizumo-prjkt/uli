#ifndef ULI_RUNTIME_LANG_EXPORT_HPP
#define ULI_RUNTIME_LANG_EXPORT_HPP

#include <string>
#include <unordered_map>

// Language translation preprocessor macro definition.
// Checks if `LANG_MAP` is initialized inside the executing scope, else returns the raw english string fallback.
#define _tr(key) ((uli::runtime::contents::translations::active_dict.find(key) != uli::runtime::contents::translations::active_dict.end()) \
                     ? uli::runtime::contents::translations::active_dict[key] : (key))

namespace uli {
namespace runtime {
namespace contents {
namespace translations {

// Globally active string dictionary mapped by load_language()
inline std::unordered_map<std::string, std::string> active_dict;

} // namespace translations
} // namespace contents
} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_LANG_EXPORT_HPP
