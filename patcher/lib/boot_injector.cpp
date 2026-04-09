#include "boot_injector.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace uli {
namespace patcher {

std::string BootInjector::read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool BootInjector::write_file(const std::string& path, const std::string& content) {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << content;
    f.close();
    return true;
}

std::string BootInjector::generate_grub_entry(const BootEntryInfo& base_entry) {
    std::string entry;
    entry += "\n\n";
    entry += "# ========================================\n";
    entry += "# ULI Installer — Injected by uli_patcher\n";
    entry += "# ========================================\n";
    entry += "menuentry \"Install with ULI Installer\" --class uli --class linux {\n";

    if (!base_entry.kernel_path.empty()) {
        entry += "    linux " + base_entry.kernel_path;
        if (!base_entry.append_params.empty()) {
            // Clone existing params and append the ULI autostart flag
            std::string params = base_entry.append_params;
            // Remove any existing quiet/splash for the installer (we want verbose)
            // Keep the essential params like archisobasedir, archisolabel, etc.
            entry += " " + params;
        }
        entry += " uli_autostart=1";
        entry += "\n";
    }

    if (!base_entry.initrd_path.empty()) {
        entry += "    initrd " + base_entry.initrd_path + "\n";
    }

    entry += "}\n";
    return entry;
}

std::string BootInjector::generate_isolinux_entry(const BootEntryInfo& base_entry) {
    std::string entry;
    entry += "\n";
    entry += "# ULI Installer — Injected by uli_patcher\n";
    entry += "LABEL uli_install\n";
    entry += "    MENU LABEL ^Install with ULI Installer\n";

    if (!base_entry.kernel_path.empty()) {
        entry += "    KERNEL " + base_entry.kernel_path + "\n";
    }

    if (!base_entry.append_params.empty()) {
        // Clone existing APPEND line with uli_autostart flag added
        std::string params = base_entry.append_params;
        entry += "    APPEND " + params + " uli_autostart=1\n";
    } else if (!base_entry.initrd_path.empty()) {
        entry += "    APPEND initrd=" + base_entry.initrd_path + " uli_autostart=1\n";
    }

    return entry;
}

std::string BootInjector::generate_systemd_boot_entry(const BootEntryInfo& base_entry) {
    std::string entry;
    entry += "title   Arch Linux with ULI Installer\n";
    
    if (!base_entry.kernel_path.empty()) {
        entry += "linux   " + base_entry.kernel_path + "\n";
    }
    
    if (!base_entry.initrd_path.empty()) {
        entry += "initrd  " + base_entry.initrd_path + "\n";
    }
    
    if (!base_entry.append_params.empty()) {
        entry += "options " + base_entry.append_params + " uli_autostart=1\n";
    } else {
        entry += "options uli_autostart=1\n";
    }
    
    return entry;
}

int BootInjector::inject_all(IsoBackend& backend,
                             const std::vector<BootConfig>& configs,
                             const std::string& temp_dir) {
    int success_count = 0;

    for (const auto& cfg : configs) {
        std::cout << "[BootInjector] Processing: " << cfg.iso_path
                  << " (" << cfg.type_str() << ")" << std::endl;

        // 1. Extract the config file from the ISO
        std::string temp_config = temp_dir + "/boot_config_" + std::to_string(success_count);
        fs::create_directories(fs::path(temp_config).parent_path());

        auto extract_res = backend.extract_file(cfg.iso_path, temp_config);
        if (!extract_res.success) {
            std::cerr << "[BootInjector] Failed to extract " << cfg.iso_path
                      << ": " << extract_res.error << std::endl;
            continue;
        }

        // 2. Read and parse the config
        std::string content = read_file(temp_config);
        if (content.empty()) {
            std::cerr << "[BootInjector] Extracted config is empty: " << cfg.iso_path << std::endl;
            continue;
        }

        // Check if we've already injected
        if (content.find("uli_autostart=1") != std::string::npos) {
            std::cout << "[BootInjector] ULI entry already present in " << cfg.iso_path
                      << ", skipping." << std::endl;
            success_count++;
            continue;
        }

        // 3. Parse existing boot entry to clone kernel/initrd
        BootEntryInfo base_entry;
        std::string new_entry;

        if (cfg.type == BootConfigType::GRUB) {
            base_entry = BootDetector::parse_grub_entry(content);
            new_entry = generate_grub_entry(base_entry);
        } else if (cfg.type == BootConfigType::ISOLINUX) {
            base_entry = BootDetector::parse_isolinux_entry(content);
            new_entry = generate_isolinux_entry(base_entry);
        } else if (cfg.type == BootConfigType::SYSTEMD_BOOT) {
            base_entry = BootDetector::parse_systemd_boot_entry(content);
            new_entry = generate_systemd_boot_entry(base_entry);
            
            // For systemd-boot, we ALSO patch the EFI hidden image if possible
            if (!new_entry.empty()) {
                if (patch_efi_image(backend, new_entry, temp_dir)) {
                    std::cout << "[BootInjector] Success: Patched hidden EFI boot image." << std::endl;
                } else {
                    std::cerr << "[BootInjector] Warning: Could not patch hidden EFI boot image. UEFI boot entries may not show." << std::endl;
                }
            }
        }

        if (base_entry.kernel_path.empty()) {
            std::cerr << "[BootInjector] Could not parse any kernel entry from "
                      << cfg.iso_path << ", skipping." << std::endl;
            continue;
        }

        // 4. Handle injection (append to file or create new file)
        std::string target_iso_path = cfg.iso_path;
        if (cfg.type == BootConfigType::SYSTEMD_BOOT) {
            // For systemd-boot, we create a new entry file instead of appending
            target_iso_path = "/loader/entries/00-uli.conf";
            content = new_entry;
        } else {
            // For GRUB/Isolinux, we append to the single config file
            content += new_entry;
        }

        // 5. Write modified config back
        std::string modified_config = temp_dir + "/modified_boot_" + std::to_string(success_count);
        if (!write_file(modified_config, content)) {
            std::cerr << "[BootInjector] Failed to write modified config." << std::endl;
            continue;
        }

        // 6. Re-inject into the ISO
        auto inject_res = backend.inject_file(modified_config, target_iso_path);
        if (!inject_res.success) {
            std::cerr << "[BootInjector] Failed to re-inject " << target_iso_path
                      << ": " << inject_res.error << std::endl;
            continue;
        }

        std::cout << "[BootInjector] Successfully injected ULI entry into " << target_iso_path << std::endl;
        success_count++;
    }

    return success_count;
}

bool BootInjector::patch_efi_image(IsoBackend& backend,
                                 const std::string& entry_content,
                                 const std::string& temp_dir) {
    // 1. Extract the original EFI image
    std::string efi_img_path = temp_dir + "/efiboot.img";
    auto extract_res = backend.extract_boot_image(efi_img_path);
    if (!extract_res.success) {
        return false;
    }

    // 2. Write the entry content to a temporary file for mcopy
    std::string uli_conf = temp_dir + "/00-uli.conf";
    if (!write_file(uli_conf, entry_content)) return false;

    // 3. Use mtools to inject the file into the FAT image
    // MTOOLS_SKIP_CHECK=1 and -D s ensure non-interactive, stable execution
    std::string env = "MTOOLS_SKIP_CHECK=1 ";
    
    // Ensure loader/entries directory exists in the image (ignore errors if it already does)
    std::string mmd_cmd = env + "mmd -D s -i " + efi_img_path + " ::/loader 2>/dev/null";
    std::system(mmd_cmd.c_str());
    mmd_cmd = env + "mmd -D s -i " + efi_img_path + " ::/loader/entries 2>/dev/null";
    std::system(mmd_cmd.c_str());

    // Copy the uli entry with overwrite flag
    std::string mcopy_cmd = env + "mcopy -o -D s -i " + efi_img_path + " " + uli_conf + 
                           " ::/loader/entries/00-uli.conf 2>/dev/null";
    int ret = std::system(mcopy_cmd.c_str());
    
    if (ret != 0) {
        std::cerr << "[BootInjector] mcopy failed for EFI image (exit " << ret << ")" << std::endl;
        return false;
    }

    // 4. Register the modified image with the backend
    backend.set_efi_boot_image(efi_img_path);
    std::cout << "[BootInjector] Successfully patched hidden EFI boot image." << std::endl;
    return true;
}

} // namespace patcher
} // namespace uli
