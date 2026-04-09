#ifndef ULI_PARTITIONER_PARTDISK_HPP
#define ULI_PARTITIONER_PARTDISK_HPP

#include <string>
#include <iostream>
#include "../sgdisk/sgdisk_wrapper.hpp"
#include "../fdisk/fdisk_wrapper.hpp"

namespace uli {
namespace partitioner {
namespace partdisk {

class PartDisk {
public:
    // Abstract top-level format command aiming to prepare a modern UEFI partition schema
    static bool prepare_efi_layout(const std::string& target_disk) {
        std::cout << "\n[PartDisk] Preparing EFI/GPT Layout for target: " << target_disk << std::endl;
        
        // 1. Initialize GPT Table using sgdisk
        if (!sgdisk::SgdiskWrapper::create_gpt_table(target_disk)) {
            std::cerr << "[PartDisk: Error] Failed to generate GPT map on " << target_disk << std::endl;
            return false;
        }
        
        // 2. Create EFI System Partition (approx 512MB) -> Code 'ef00'
        // '0' as start uses largest available early sector, '+512M' is chunk length
        if (!sgdisk::SgdiskWrapper::create_partition(target_disk, 1, "0", "+512M", "ef00")) {
            std::cerr << "[PartDisk: Error] Failed creating EFI partition." << std::endl;
            return false;
        }

        // 3. Create Root Partition (Rest of the disk) -> Code '8300' (Linux filesystem)
        // '0' as start dynamically chains after partition 1. '0' for end consumes remainder.
        if (!sgdisk::SgdiskWrapper::create_partition(target_disk, 2, "0", "0", "8300")) {
            std::cerr << "[PartDisk: Error] Failed creating Linux Root partition." << std::endl;
            return false;
        }

        sgdisk::SgdiskWrapper::refresh_part_table(target_disk);
        std::cout << "[PartDisk] Successfully mapped dynamic UEFI layout partitions block devices.\n";
        return true;
    }
};

} // namespace partdisk
} // namespace partitioner
} // namespace uli

#endif // ULI_PARTITIONER_PARTDISK_HPP
