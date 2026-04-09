#ifndef ULI_PARTITIONER_SGDISK_WRAPPER_HPP
#define ULI_PARTITIONER_SGDISK_WRAPPER_HPP

#include <string>
#include <iostream>
#include <cstdlib>

namespace uli {
namespace partitioner {
namespace sgdisk {

class SgdiskWrapper {
public:
    // Wipes the disk and creates a new GUID Partition Table (GPT)
    static bool create_gpt_table(const std::string& disk_path) {
        std::cout << "[sgdisk] Creating GPT partition table on " << disk_path << std::endl;
        // -Z zaps the disk (destroys structures), -o creates a new protective MBR and empty GPT
        std::string cmd = "sgdisk -Z -o " + disk_path + " > /dev/null 2>&1";
        return (std::system(cmd.c_str()) == 0);
    }
    
    // Creates a partition. 'part_num' is the structural index, 'type_code' is the EFI hex (e.g. ef00 for EFI, 8300 for Linux)
    static bool create_partition(const std::string& disk_path, int part_num, const std::string& size_start, const std::string& size_end, const std::string& type_code) {
        std::cout << "[sgdisk] Writing partition " << part_num << " [" << type_code << "] to " << disk_path << std::endl;
        // sgdisk -n partnum:start:end (0 or empty means default/remaining space), -t specifies type code
        // A size_end of "0" means "use all remaining space" — pass empty to sgdisk for that
        std::string effective_end = (size_end == "0") ? "" : size_end;
        std::string cmd = "sgdisk -n " + std::to_string(part_num) + ":" + size_start + ":" + effective_end 
                        + " -t " + std::to_string(part_num) + ":" + type_code + " " + disk_path + " > /dev/null 2>&1";
        return (std::system(cmd.c_str()) == 0);
    }
    
    // Forces the kernel to re-read the partition table
    static void refresh_part_table(const std::string& disk_path) {
        std::string cmd = "partprobe " + disk_path + " > /dev/null 2>&1";
        std::system(cmd.c_str());
    }
};

} // namespace sgdisk
} // namespace partitioner
} // namespace uli

#endif // ULI_PARTITIONER_SGDISK_WRAPPER_HPP
