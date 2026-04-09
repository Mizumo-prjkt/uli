#ifndef ULI_PATCHER_BOOT_DETECTOR_HPP
#define ULI_PATCHER_BOOT_DETECTOR_HPP

#include <string>
#include <vector>
#include "iso_backend.hpp"

namespace uli {
namespace patcher {

enum class BootConfigType {
    GRUB,           // GRUB2 (UEFI or BIOS)
    ISOLINUX,       // Isolinux/Syslinux (BIOS)
    SYSTEMD_BOOT,   // systemd-boot (UEFI, common on Arch)
    UNKNOWN
};

struct BootConfig {
    BootConfigType type;
    std::string iso_path;       // Path inside the ISO (e.g., "/boot/grub/grub.cfg")
    std::string display_name;   // Human-readable name

    std::string type_str() const {
        switch (type) {
            case BootConfigType::GRUB: return "GRUB";
            case BootConfigType::ISOLINUX: return "Isolinux/Syslinux";
            case BootConfigType::SYSTEMD_BOOT: return "systemd-boot";
            default: return "Unknown";
        }
    }
};

// Kernel/initrd path extracted from existing boot entries
struct BootEntryInfo {
    std::string kernel_path;    // e.g., "/arch/boot/x86_64/vmlinuz-linux"
    std::string initrd_path;    // e.g., "/arch/boot/x86_64/initramfs-linux.img"
    std::string append_params;  // Extra kernel params from existing entry
};

class BootDetector {
public:
    // Scan the ISO for known boot config files
    static std::vector<BootConfig> detect(IsoBackend& backend);

    // Parse an existing GRUB config to extract kernel/initrd paths
    static BootEntryInfo parse_grub_entry(const std::string& config_content);

    // Parse an existing Isolinux/Syslinux config to extract kernel/initrd paths
    static BootEntryInfo parse_isolinux_entry(const std::string& config_content);
    
    // Parse an existing systemd-boot config entry to extract kernel/initrd paths
    static BootEntryInfo parse_systemd_boot_entry(const std::string& config_content);

private:
    // Known boot config paths to probe
    static const std::vector<std::pair<std::string, BootConfigType>> known_paths_;
};

} // namespace patcher
} // namespace uli

#endif // ULI_PATCHER_BOOT_DETECTOR_HPP
