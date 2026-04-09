#include "boot_detector.hpp"
#include <iostream>
#include <sstream>
#include <regex>
#include <algorithm>

namespace uli {
namespace patcher {

// Known boot configuration paths inside Linux ISOs
const std::vector<std::pair<std::string, BootConfigType>> BootDetector::known_paths_ = {
    // GRUB configs (UEFI and BIOS)
    {"/boot/grub/grub.cfg",        BootConfigType::GRUB},
    {"/EFI/BOOT/grub.cfg",         BootConfigType::GRUB},
    {"/boot/grub/loopback.cfg",    BootConfigType::GRUB},
    {"/grub/grub.cfg",             BootConfigType::GRUB},

    // Isolinux/Syslinux configs (BIOS)
    {"/isolinux/isolinux.cfg",     BootConfigType::ISOLINUX},
    {"/syslinux/syslinux.cfg",     BootConfigType::ISOLINUX},
    {"/boot/syslinux/syslinux.cfg",BootConfigType::ISOLINUX},
    {"/isolinux/txt.cfg",          BootConfigType::ISOLINUX},
    {"/boot/isolinux/isolinux.cfg",BootConfigType::ISOLINUX},

    // Systemd-boot (UEFI entries)
    {"/loader/entries/01-archiso-linux.conf", BootConfigType::SYSTEMD_BOOT},
    {"/loader/entries/archiso-x86_64-linux.conf", BootConfigType::SYSTEMD_BOOT},
};

std::vector<BootConfig> BootDetector::detect(IsoBackend& backend) {
    std::vector<BootConfig> found;

    std::cout << "[BootDetector] Scanning ISO for boot configurations..." << std::endl;
    for (const auto& [path, type] : known_paths_) {
        std::cout << "[BootDetector] Probing: " << path << std::endl;
        if (backend.exists(path)) {
            BootConfig cfg;
            cfg.type = type;
            cfg.iso_path = path;
            cfg.display_name = path;

            std::cout << "[BootDetector]   Found: " << path
                      << " (" << cfg.type_str() << ")" << std::endl;
            found.push_back(cfg);
        }
    }

    if (found.empty()) {
        std::cout << "[BootDetector]   No known boot configurations detected." << std::endl;
    }

    return found;
}

BootEntryInfo BootDetector::parse_grub_entry(const std::string& content) {
    BootEntryInfo info;

    // Find the first menuentry block and extract linux/initrd lines
    // Pattern: linux /path/to/vmlinuz <params>
    std::regex linux_re(R"(^\s*linux\w*\s+(/\S+))");
    std::regex initrd_re(R"(^\s*initrd\w*\s+(/\S+))");

    std::istringstream iss(content);
    std::string line;
    bool in_menuentry = false;

    while (std::getline(iss, line)) {
        if (line.find("menuentry") != std::string::npos) {
            in_menuentry = true;
            continue;
        }

        if (in_menuentry) {
            std::smatch match;

            if (info.kernel_path.empty() && std::regex_search(line, match, linux_re)) {
                info.kernel_path = match[1].str();

                // Extract everything after the kernel path as append params
                auto kernel_end = line.find(info.kernel_path) + info.kernel_path.length();
                if (kernel_end < line.length()) {
                    info.append_params = line.substr(kernel_end);
                    // Trim leading whitespace
                    auto start = info.append_params.find_first_not_of(" \t");
                    if (start != std::string::npos) {
                        info.append_params = info.append_params.substr(start);
                    } else {
                        info.append_params.clear();
                    }
                }
            }

            if (info.initrd_path.empty() && std::regex_search(line, match, initrd_re)) {
                info.initrd_path = match[1].str();
            }

            // We only need the first entry
            if (!info.kernel_path.empty() && !info.initrd_path.empty()) {
                break;
            }
        }
    }

    if (!info.kernel_path.empty()) {
        std::cout << "[BootDetector] Parsed GRUB entry:"
                  << "\n    kernel:  " << info.kernel_path
                  << "\n    initrd:  " << info.initrd_path
                  << "\n    params:  " << info.append_params << std::endl;
    }

    return info;
}

BootEntryInfo BootDetector::parse_isolinux_entry(const std::string& content) {
    BootEntryInfo info;

    // Pattern: KERNEL /path/to/vmlinuz  and  APPEND initrd=/path/to/initrd <params>
    std::istringstream iss(content);
    std::string line;
    bool in_label = false;

    while (std::getline(iss, line)) {
        // Trim whitespace
        auto start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        std::string trimmed = line.substr(start);

        // Convert to uppercase for case-insensitive matching
        std::string upper = trimmed;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

        if (upper.find("LABEL ") == 0) {
            in_label = true;
            continue;
        }

        if (in_label) {
            if (upper.find("KERNEL ") == 0 && info.kernel_path.empty()) {
                info.kernel_path = trimmed.substr(7);
                // Trim
                auto ks = info.kernel_path.find_first_not_of(" \t");
                if (ks != std::string::npos) info.kernel_path = info.kernel_path.substr(ks);
            }

            if (upper.find("APPEND ") == 0) {
                std::string append_line = trimmed.substr(7);
                // Extract initrd= from APPEND
                auto initrd_pos = append_line.find("initrd=");
                if (initrd_pos != std::string::npos) {
                    auto path_start = initrd_pos + 7;
                    auto path_end = append_line.find(' ', path_start);
                    info.initrd_path = append_line.substr(path_start, 
                        path_end != std::string::npos ? path_end - path_start : std::string::npos);
                }
                info.append_params = append_line;
            }

            // Got what we need from first label block
            if (!info.kernel_path.empty() && !info.append_params.empty()) {
                break;
            }
        }
    }

    if (!info.kernel_path.empty()) {
        std::cout << "[BootDetector] Parsed Isolinux entry:"
                  << "\n    kernel: " << info.kernel_path
                  << "\n    initrd: " << info.initrd_path
                  << "\n    append: " << info.append_params << std::endl;
    }

    return info;
}

BootEntryInfo BootDetector::parse_systemd_boot_entry(const std::string& content) {
    BootEntryInfo info;

    // Patterns: 
    // linux /path
    // initrd /path
    // options <params>
    std::istringstream iss(content);
    std::string line;

    while (std::getline(iss, line)) {
        auto start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        std::string trimmed = line.substr(start);

        if (trimmed.find("linux ") == 0) {
            info.kernel_path = trimmed.substr(6);
            auto s = info.kernel_path.find_first_not_of(" \t");
            if (s != std::string::npos) info.kernel_path = info.kernel_path.substr(s);
        } else if (trimmed.find("initrd ") == 0) {
            info.initrd_path = trimmed.substr(7);
            auto s = info.initrd_path.find_first_not_of(" \t");
            if (s != std::string::npos) info.initrd_path = info.initrd_path.substr(s);
        } else if (trimmed.find("options ") == 0) {
            info.append_params = trimmed.substr(8);
            auto s = info.append_params.find_first_not_of(" \t");
            if (s != std::string::npos) info.append_params = info.append_params.substr(s);
        }
    }

    if (!info.kernel_path.empty()) {
        std::cout << "[BootDetector] Parsed Systemd-boot entry:"
                  << "\n    kernel:  " << info.kernel_path
                  << "\n    initrd:  " << info.initrd_path
                  << "\n    options: " << info.append_params << std::endl;
    }

    return info;
}

} // namespace patcher
} // namespace uli
