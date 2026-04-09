#ifndef ULI_APK_MGR_HPP
#define ULI_APK_MGR_HPP

#include "../../packagemanager_layer_interface.hpp"
#include "../../runtime/blackbox.hpp"
#include <iostream>
#include <sstream>

namespace uli {
namespace package_mgr {
namespace apk {

class ApkManager : public PackageManagerInterface {
public:
    ApkManager() = default;
    ~ApkManager() override = default;

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
        
        std::string cmd = "apk search -q '^" + query + "' 2>/dev/null";
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
        // If we are installing to /mnt, we use --root
        bool is_bootstrap = (std::getenv("ULI_BOOTSTRAP") != nullptr);
        if (is_bootstrap) {
            cmd << "apk add --root /mnt --initdb";
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

    bool is_synced() override {
        // Check for presence of apk cache or metadata
        std::string cmd = "ls /var/cache/apk/ | wc -l";
        std::string output = exec_command(cmd.c_str());
        try {
            int count = std::stoi(output);
            return count > 0;
        } catch (...) {
            return false;
        }
    }

    bool sync_system() override {
        std::cout << "[INFO] Synchronizing Alpine Linux package repositories (apk update)..." << std::endl;
        uli::runtime::BlackBox::log("PM_SYNC: Synchronizing Alpine repositories (apk update)");
        int ret = std::system("apk update >/dev/null 2>&1");
        bool success = (ret == 0);
        if (!success) uli::runtime::BlackBox::log("PM_SYNC_FAIL: apk update failed");
        else uli::runtime::BlackBox::log("PM_SYNC_SUCCESS");
        return success;
    }

    bool configure_mirrors(const std::vector<std::string>& mirror_urls) override {
        if (mirror_urls.empty()) return true;
        if (mirror_urls.size() == 1 && (mirror_urls[0] == "Default" || mirror_urls[0] == "http://dl-cdn.alpinelinux.org/alpine/")) {
            return true;
        }
        
        // Alpine mirrors are injected into /etc/apk/repositories
        std::string list_replace = "";
        for (size_t i = 0; i < mirror_urls.size(); ++i) {
            std::string safe_url = mirror_urls[i];
            if (safe_url.back() != '/') safe_url += "/";
            if (i > 0) list_replace += "\\n";
            list_replace += "\\1" + safe_url + "\\2";
        }
        
        std::string sed_cmd = "sed -i -e 's|^\\(.*\\)http://dl-cdn.alpinelinux.org/alpine/\\(.*\\)|" + list_replace + "|g' /etc/apk/repositories 2>/dev/null";
        int ret = std::system(sed_cmd.c_str());
        
        std::system("apk update >/dev/null 2>&1");
        return true; 
    }
};

}
}
}

#endif
