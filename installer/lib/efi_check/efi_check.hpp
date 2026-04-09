#ifndef ULI_EFI_CHECK_EFI_CHECK_HPP
#define ULI_EFI_CHECK_EFI_CHECK_HPP

#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace uli {
namespace efi_check {

class EFICheck {
public:
    // Returns true if the system was booted via EFI, determined by checking if /sys/firmware/efi exists.
    static bool is_efi_system() {
        struct stat buffer;
        return (stat("/sys/firmware/efi", &buffer) == 0);
    }
};

} // namespace efi_check
} // namespace uli

#endif // ULI_EFI_CHECK_EFI_CHECK_HPP
