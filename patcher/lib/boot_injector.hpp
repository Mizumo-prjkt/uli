#ifndef ULI_PATCHER_BOOT_INJECTOR_HPP
#define ULI_PATCHER_BOOT_INJECTOR_HPP

#include <string>
#include <vector>
#include "iso_backend.hpp"
#include "boot_detector.hpp"

namespace uli {
namespace patcher {

class BootInjector {
public:
    // Inject ULI boot entries into all detected boot configs
    // Returns number of configs successfully modified
    static int inject_all(IsoBackend& backend,
                          const std::vector<BootConfig>& configs,
                          const std::string& temp_dir);

    // Patch a hidden EFI FAT image using mtools
    static bool patch_efi_image(IsoBackend& backend,
                                const std::string& entry_content,
                                const std::string& temp_dir);

    // Generate the GRUB menuentry block for ULI
    static std::string generate_grub_entry(const BootEntryInfo& base_entry);

    // Generate the Isolinux/Syslinux LABEL block for ULI
    static std::string generate_isolinux_entry(const BootEntryInfo& base_entry);

    // Generate the systemd-boot entry content for ULI
    static std::string generate_systemd_boot_entry(const BootEntryInfo& base_entry);

private:
    // Read file content from disk path
    static std::string read_file(const std::string& path);

    // Write file content to disk path
    static bool write_file(const std::string& path, const std::string& content);
};

} // namespace patcher
} // namespace uli

#endif // ULI_PATCHER_BOOT_INJECTOR_HPP
