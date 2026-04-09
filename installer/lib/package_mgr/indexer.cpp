#include "indexer.hpp"

#include "../runtime/menu.hpp"
#include <iomanip>

namespace uli {
namespace package_mgr {

std::vector<std::string> PackageIndexer::get_packages_from_map(
    const TranslationMap& package_map, 
    const std::string& os_distro,
    const std::string& fallback_generic_name) 
{
    auto distro_it = package_map.find(os_distro);
    if (distro_it != package_map.end()) {
        return distro_it->second; // Exact match found for this distro
    }
    
    // Exact distro not found, return empty or fallback
    
    // Fallback: If no translation exists, return the generic name as the literal best guess
    if (!fallback_generic_name.empty()) {
        return {fallback_generic_name};
    }
    
    return {};
}

std::string PackageIndexer::resolve_package_name(
    const std::string& generic_name,
    const std::string& os_distro,
    const std::vector<uli::runtime::ManualMapping>& manual_mappings) 
{
    // 1. Check user-defined manual overrides first
    for (const auto& m : manual_mappings) {
        if (m.generic_name == generic_name) {
            if (os_distro == "Arch Linux" && !m.arch_pkg.empty()) return m.arch_pkg;
            if (os_distro == "Alpine Linux" && !m.alpine_pkg.empty()) return m.alpine_pkg;
            if (os_distro == "Debian" && !m.debian_pkg.empty()) return m.debian_pkg;
        }
    }

    // 2. Default to the generic name if no override exists
    return generic_name;
}

} // namespace package_mgr
} // namespace uli