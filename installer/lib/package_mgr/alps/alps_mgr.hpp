#ifndef ULI_ALPS_MGR_HPP
#define ULI_ALPS_MGR_HPP

#include "../../packagemanager_layer_interface.hpp"
#include "mirrorlist.hpp"
#include <iostream>
#include <sstream>

namespace uli {
namespace package_mgr {
namespace alps {

class AlpsManager : public PackageManagerInterface {
public:
    AlpsManager() = default;
    ~AlpsManager() override = default;

    bool load_translation(const std::string& filepath) override { 
        try {
            YAML::Node yaml_doc = YAML::LoadFile(filepath);
            parse_yaml_node(yaml_doc);
            return true;
        } catch (const YAML::Exception& e) {
            std::cerr << "YAML Error loading translation dictionary " << filepath << ": " << e.what() << std::endl;
            return false;
        }
    }

    bool load_translation_from_string(const std::string& yaml_str) override {
        try {
            YAML::Node yaml_doc = YAML::Load(yaml_str);
            parse_yaml_node(yaml_doc);
            return true;
        } catch (const YAML::Exception& e) {
            std::cerr << "YAML Error loading builtin translation dictionary: " << e.what() << std::endl;
            return false;
        }
    }
    
    std::vector<std::string> search_packages(const std::string& query) override {
        std::vector<std::string> results;
        if (query.empty()) return results;
        
        std::string cmd = "pacman -Ssq '^" + query + "' 2>/dev/null";
        std::string output = exec_command(cmd.c_str());
        
        if (output == "_ULI_PM_ERROR_") {
            return {"_ULI_PM_ERROR_"};
        }
        
        std::istringstream stream(output);
        std::string line;
        while (std::getline(stream, line)) {
            if (!line.empty()) {
                results.push_back(line);
            }
        }
        return results;
    }

    std::string build_install_command(const std::vector<std::string>& abstract_packages) const override { 
        std::stringstream cmd;
        // If we are installing to /mnt, we use pacstrap
        bool is_bootstrap = (std::getenv("ULI_BOOTSTRAP") != nullptr);
        if (is_bootstrap) {
            cmd << "pacstrap -K /mnt";
        } else {
            cmd << config.install_cmd;
        }

        for (const auto& pkg : abstract_packages) {
            cmd << " " << translate_package(pkg);
        }
        return cmd.str();
    }

    std::string build_remove_command(const std::vector<std::string>& abstract_packages) const override { 
        std::stringstream cmd;
        cmd << config.remove_cmd;
        for (const auto& pkg : abstract_packages) {
            cmd << " " << translate_package(pkg);
        }
        return cmd.str();
    }
    bool is_available() const override { return true; }

    bool configure_mirrors(const std::vector<std::string>& mirror_urls) override {
        if (mirror_urls.empty()) return true;
        
        if (mirror_urls.size() == 1 && mirror_urls[0] == "Default") {
            return true;
        }
        
        if (mirror_urls.size() == 1 && mirror_urls[0] == "Auto-Optimize (Reflector)") {
            return alps::Mirrorlist::optimize_mirrors();
        }
        
        // Custom URL Overwrite Loop
        std::system("rm -f /etc/pacman.d/mirrorlist && touch /etc/pacman.d/mirrorlist");
        for (const auto& url : mirror_urls) {
            std::string safe_url = url;
            if (safe_url.empty()) continue;
            std::string cmd = "echo 'Server = " + safe_url + "' >> /etc/pacman.d/mirrorlist";
            int ret = std::system(cmd.c_str());
            (void)ret;
        }
        
        return true;
    }
};

}
}
}

#endif
