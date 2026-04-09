#ifndef ULI_PACKAGE_MGR_LINTER_PKG_HPP
#define ULI_PACKAGE_MGR_LINTER_PKG_HPP

#include <string>
#include <vector>
#include <algorithm>
#include "../packagemanager_layer_interface.hpp"

namespace uli {
namespace package_mgr {

enum class LintResult {
    EXISTS,
    NOT_FOUND,
    PM_UNAVAILABLE
};

class PackageLinter {
public:
    static LintResult verify_package(PackageManagerInterface* pm, const std::string& pkg_name) {
        if (!pm || pkg_name.empty()) return LintResult::PM_UNAVAILABLE;
        
        std::vector<std::string> results = pm->search_packages(pkg_name);
        
        if (!results.empty() && results[0] == "_ULI_PM_ERROR_") {
            return LintResult::PM_UNAVAILABLE;
        }

        // We check for exact string match within the returned array to explicitly
        // prevent substring false positives (e.g. searching "nano" returning "nano-syntax").
        for (const auto& found_pkg : results) {
            if (found_pkg == pkg_name) {
                return LintResult::EXISTS;
            }
        }
        
        return LintResult::NOT_FOUND;
    }
};

} // namespace package_mgr
} // namespace uli

#endif // ULI_PACKAGE_MGR_LINTER_PKG_HPP
