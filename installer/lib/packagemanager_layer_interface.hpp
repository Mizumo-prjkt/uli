#ifndef ULI_PACKAGEMANAGER_LAYER_INTERFACE_HPP
#define ULI_PACKAGEMANAGER_LAYER_INTERFACE_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <yaml-cpp/yaml.h>
#include <memory>
#include <array>
#include <cstdio>

namespace uli {
namespace package_mgr {

struct PackageManagerConfig {
    std::string binary;
    std::string update_cmd;
    std::string install_cmd;
    std::string remove_cmd;
    std::string clean_cmd;
};

class PackageManagerInterface {
protected:
    PackageManagerConfig config;
    std::unordered_map<std::string, std::string> package_map;

    // Common logic to parse YAML translation nodes
    void parse_yaml_node(const YAML::Node& yaml_doc) {
        if (!yaml_doc) return;

        // Load Package Manager configs if present
        if (yaml_doc["package_manager"]) {
            auto pm = yaml_doc["package_manager"];
            if (pm["binary"]) config.binary = pm["binary"].as<std::string>();
            if (pm["update_cmd"]) config.update_cmd = pm["update_cmd"].as<std::string>();
            if (pm["install_cmd"]) config.install_cmd = pm["install_cmd"].as<std::string>();
            if (pm["remove_cmd"]) config.remove_cmd = pm["remove_cmd"].as<std::string>();
            if (pm["clean_cmd"]) config.clean_cmd = pm["clean_cmd"].as<std::string>();
        }

        // Load Package translation map
        if (yaml_doc["package_map"]) {
            auto pm_map = yaml_doc["package_map"];
            for (auto it = pm_map.begin(); it != pm_map.end(); ++it) {
                package_map[it->first.as<std::string>()] = it->second.as<std::string>();
            }
        }
    }

    std::string exec_command(const char* cmd) const {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe) {
            return "_ULI_PM_ERROR_";
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

public:
    virtual ~PackageManagerInterface() = default;

    // Optional method for package managers that support runtime mirror reconfiguration
    virtual bool configure_mirrors(const std::vector<std::string>& mirror_urls) = 0;

    // Search packages dynamically returning an array array of results
    virtual std::vector<std::string> search_packages(const std::string& query) = 0;

    // Load translation map from trans.yaml
    virtual bool load_translation(const std::string& filepath) = 0;

    // Load translation map directly from a YAML string (for builtin fallbacks)
    virtual bool load_translation_from_string(const std::string& yaml_str) = 0;

    // Get the native distribution package mapped from standard ULI package name
    virtual std::string translate_package(const std::string& abstract_pkg) const {
        auto it = package_map.find(abstract_pkg);
        if (it != package_map.end()) {
            return it->second;
        }
        return abstract_pkg;
    }

    // Build complete install command for a list of abstract packages
    virtual std::string build_install_command(const std::vector<std::string>& abstract_packages) const = 0;

    // Build complete remove command
    virtual std::string build_remove_command(const std::vector<std::string>& abstract_packages) const = 0;

    // Build complete update command
    virtual std::string build_update_command() const { return config.update_cmd; }

    // Build complete clean command
    virtual std::string build_clean_command() const { return config.clean_cmd; }

    // Check if the package manager binary exists on the system
    virtual bool is_available() const = 0;
};

} // namespace package_mgr
} // namespace uli

#endif // ULI_PACKAGEMANAGER_LAYER_INTERFACE_HPP
