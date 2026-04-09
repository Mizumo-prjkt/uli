#ifndef ULI_DPKG_APT_HPP
#define ULI_DPKG_APT_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <yaml-cpp/yaml.h>
#include "../../packagemanager_layer_interface.hpp"

namespace uli {
namespace package_mgr {

class DpkgAptManager : public PackageManagerInterface {
public:
    DpkgAptManager();
    ~DpkgAptManager() override = default;

    // Optional method for package managers that support runtime mirror reconfiguration
    bool configure_mirrors(const std::vector<std::string>& mirror_urls) override;

    // Load translation map from trans.yaml
    bool load_translation(const std::string& filepath) override;

    // Load translation map directly from a YAML string (for builtin fallbacks)
    bool load_translation_from_string(const std::string& yaml_str) override;

    // Search Packages
    std::vector<std::string> search_packages(const std::string& query) override;

    // Build complete install command for a list of abstract packages
    std::string build_install_command(const std::vector<std::string>& abstract_packages) const override;

    // Build complete remove command
    std::string build_remove_command(const std::vector<std::string>& abstract_packages) const override;

    // Check if the package manager binary exists on the system
    bool is_available() const override;
};



} // namespace package_mgr
} // namespace uli

#endif // ULI_DPKG_APT_HPP
