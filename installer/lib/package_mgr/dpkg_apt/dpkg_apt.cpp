#include "dpkg_apt.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>

namespace uli {
namespace package_mgr {

DpkgAptManager::DpkgAptManager() {
    // defaults
    config.binary = "apt-get";
    config.update_cmd = "apt-get update";
    config.install_cmd = "DEBIAN_FRONTEND=noninteractive apt-get install -y";
    config.remove_cmd = "DEBIAN_FRONTEND=noninteractive apt-get remove -y";
    config.clean_cmd = "apt-get autoremove -y && apt-get clean";
}

bool DpkgAptManager::configure_mirrors(const std::vector<std::string>& mirror_urls) {
    if (mirror_urls.empty()) return true;
    
    if (mirror_urls.size() == 1 && (mirror_urls[0] == "Default" || 
        mirror_urls[0] == "http://deb.debian.org/debian/" || 
        mirror_urls[0] == "http://archive.ubuntu.com/ubuntu/")) {
        return true; 
    }
    
    std::string list_replace = "";
    std::string sources_replace = "";
    
    for (size_t i = 0; i < mirror_urls.size(); ++i) {
        std::string safe_url = mirror_urls[i];
        if (safe_url.back() != '/') safe_url += "/";
        
        if (i > 0) {
            list_replace += "\\n";
            sources_replace += " ";
        }
        list_replace += "\\1" + safe_url + "\\2";
        sources_replace += safe_url;
    }
    
    // Replace standard Debian and Ubuntu repositories natively across formats
    std::string list_sed = "sed -i -e 's|^\\(.*\\)http://deb.debian.org/debian/\\(.*\\)|" + list_replace + "|g' "
                           "-e 's|^\\(.*\\)http://archive.ubuntu.com/ubuntu/\\(.*\\)|" + list_replace + "|g' "
                           "/etc/apt/sources.list /etc/apt/sources.list.d/*.list 2>/dev/null";

    std::string sources_sed = "sed -i -e 's|http://deb.debian.org/debian/|" + sources_replace + "|g' "
                              "-e 's|http://archive.ubuntu.com/ubuntu/|" + sources_replace + "|g' "
                              "/etc/apt/sources.list.d/*.sources 2>/dev/null";
                              
    int ret1 = std::system(list_sed.c_str());
    int ret2 = std::system(sources_sed.c_str());
    
    // We optionally run apt-get update to fetch from new mirrors immediately
    std::system("apt-get update >/dev/null 2>&1");
    
    return true;
}

bool DpkgAptManager::load_translation(const std::string& filepath) {
    try {
        YAML::Node yaml_doc = YAML::LoadFile(filepath);
        parse_yaml_node(yaml_doc);
        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "YAML Error loading translation dictionary " << filepath << ": " << e.what() << std::endl;
        return false;
    }
}

bool DpkgAptManager::load_translation_from_string(const std::string& yaml_str) {
    try {
        YAML::Node yaml_doc = YAML::Load(yaml_str);
        parse_yaml_node(yaml_doc);
        return true;
    } catch (const YAML::Exception& e) {
        std::cerr << "YAML Error loading builtin translation dictionary: " << e.what() << std::endl;
        return false;
    }
}

std::string DpkgAptManager::build_install_command(const std::vector<std::string>& abstract_packages) const {
    std::stringstream cmd;
    cmd << config.install_cmd;

    for (const auto& pkg : abstract_packages) {
        cmd << " " << translate_package(pkg);
    }

    return cmd.str();
}

std::string DpkgAptManager::build_remove_command(const std::vector<std::string>& abstract_packages) const {
    std::stringstream cmd;
    cmd << config.remove_cmd;

    for (const auto& pkg : abstract_packages) {
        cmd << " " << translate_package(pkg);
    }

    return cmd.str();
}

bool DpkgAptManager::is_available() const {
    std::string check_cmd = "which " + config.binary + " > /dev/null 2>&1";
    int res = system(check_cmd.c_str());
    return res == 0;
}

std::vector<std::string> DpkgAptManager::search_packages(const std::string& query) {
    std::vector<std::string> results;
    if (query.empty()) return results;
    
    // Perform apt-cache search safely
    std::string cmd = "apt-cache search --names-only '^" + query + "' 2>/dev/null";
    std::string output = exec_command(cmd.c_str());
    
    if (output == "_ULI_PM_ERROR_") {
        return {"_ULI_PM_ERROR_"};
    }
    
    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty()) {
            // apt-cache search returns "pkgname - description". Extract the package name.
            size_t space_pos = line.find(' ');
            if (space_pos != std::string::npos) {
                results.push_back(line.substr(0, space_pos));
            } else {
                results.push_back(line);
            }
        }
    }
    
    return results;
}

} // namespace package_mgr
} // namespace uli

