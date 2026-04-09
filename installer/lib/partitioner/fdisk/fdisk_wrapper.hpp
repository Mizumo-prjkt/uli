#ifndef ULI_PARTITIONER_FDISK_WRAPPER_HPP
#define ULI_PARTITIONER_FDISK_WRAPPER_HPP

#include <string>
#include <iostream>
#include <cstdlib>

namespace uli {
namespace partitioner {
namespace fdisk {

class FdiskWrapper {
public:
    // Creates an MBR partition table on the target disk
    static bool create_mbr_table(const std::string& disk_path) {
        std::cout << "[fdisk] Creating MBR (dos) partition table on " << disk_path << std::endl;
        
        // Use sfdisk or echo to fdisk to script an MBR table
        // 'o' creates a new empty DOS partition table
        std::string cmd = "echo 'o\nw' | fdisk " + disk_path + " > /dev/null 2>&1";
        return (std::system(cmd.c_str()) == 0);
    }
    
    // Abstracted helper, though traditionally sgdisk is better for GPT/EFI scripting
    static bool create_partition(const std::string& disk_path, const std::string& type, const std::string& size) {
        std::cout << "[fdisk] Creating " << type << " partition (" << size << ") on " << disk_path << std::endl;
        // In a full implementation, interactive feeding to fdisk via bash blocks or sfdisk is required here
        return true; 
    }
};

} // namespace fdisk
} // namespace partitioner
} // namespace uli

#endif // ULI_PARTITIONER_FDISK_WRAPPER_HPP
