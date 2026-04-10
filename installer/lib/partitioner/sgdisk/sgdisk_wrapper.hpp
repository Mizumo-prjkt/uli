#ifndef ULI_PARTITIONER_SGDISK_WRAPPER_HPP
#define ULI_PARTITIONER_SGDISK_WRAPPER_HPP

#include <string>
#include <iostream>
#include <cstdlib>
#include "../../runtime/blackbox.hpp"
#include <unistd.h>

namespace uli {
namespace partitioner {
namespace sgdisk {

class SgdiskWrapper {
public:
    // Wipes the disk and creates a new GUID Partition Table (GPT) with multi-tool fallbacks
    static bool create_gpt_table(const std::string& disk_path) {
        uli::runtime::BlackBox::log("ENTER create_gpt_table on " + disk_path);
        
        auto has_bin = [](const std::string& name) {
            std::string cmd = "command -v " + name + " > /dev/null 2>&1";
            return (std::system(cmd.c_str()) == 0);
        };

        // Fallback 1: Preferred (sgdisk)
        if (has_bin("sgdisk")) {
            uli::runtime::BlackBox::log("GPT INIT: Using primary tool (sgdisk)");
            std::cout << "[partitioner] Initializing GPT via sgdisk (Level 1)...\n";
            
            // Sequential robust initialization
            std::string zap = "sgdisk --zap-all \"" + disk_path + "\" > /dev/null 2>&1";
            std::string clr = "sgdisk --clear \"" + disk_path + "\" > /dev/null 2>&1";
            std::string m2g = "sgdisk --mbrtogpt \"" + disk_path + "\" > /dev/null 2>&1";
            
            if (std::system(zap.c_str()) == 0 && std::system(clr.c_str()) == 0) {
                std::system(m2g.c_str());
                return true;
            }
            uli::runtime::BlackBox::log("WARNING: sgdisk failed. Falling back to fdisk...");
        }

        // Fallback 2: Secondary (fdisk)
        if (has_bin("fdisk")) {
            uli::runtime::BlackBox::log("GPT INIT: Using fallback tool (fdisk)");
            std::cout << "[partitioner] Initializing GPT via fdisk (Level 2)...\n";
            
            // Nuclear wipe first to clear any messy residues
            std::string wipe = "dd if=/dev/zero of=\"" + disk_path + "\" bs=1M count=10 conv=notrunc > /dev/null 2>&1";
            std::system(wipe.c_str());

            // Script fdisk to create GPT
            std::string cmd = "printf \"g\\nw\\n\" | fdisk \"" + disk_path + "\" > /dev/null 2>&1";
            if (std::system(cmd.c_str()) == 0) return true;
        }

        // Fallback 3: Tertiary (parted)
        if (has_bin("parted")) {
            uli::runtime::BlackBox::log("GPT INIT: Using final fallback (parted)");
            std::cout << "[partitioner] Initializing GPT via parted (Level 3)...\n";
            std::string cmd = "parted -s \"" + disk_path + "\" mklabel gpt > /dev/null 2>&1";
            if (std::system(cmd.c_str()) == 0) return true;
        }

        uli::runtime::BlackBox::log("FATAL: All GPT initialization tools failed!");
        return false;
    }
    
    // Creates a partition. 'part_num' is the structural index, 'type_code' is the EFI hex (e.g. ef00 for EFI, 8300 for Linux)
    static bool create_partition(const std::string& disk_path, int part_num, const std::string& size_start, const std::string& size_end, const std::string& type_code) {
        std::cout << "[sgdisk] Partition " << part_num << " (" << size_end << ") -> \"" << disk_path << "\"" << std::endl;
        // sgdisk -n partnum:start:end (0 or empty means default/remaining space), -t specifies type code
        std::string effective_end = (size_end == "0") ? "" : size_end;
        std::string cmd = "sgdisk -n " + std::to_string(part_num) + ":" + size_start + ":" + effective_end 
                        + " -t " + std::to_string(part_num) + ":" + type_code + " \"" + disk_path + "\" > /dev/null 2>&1";
        uli::runtime::BlackBox::log("EXEC: " + cmd);
        return (std::system(cmd.c_str()) == 0);
    }
    
    // Forces the kernel to re-read the partition table
    static void refresh_part_table(const std::string& disk_path) {
        std::string cmd = "partprobe \"" + disk_path + "\" > /dev/null 2>&1";
        uli::runtime::BlackBox::log("EXEC: " + cmd);
        std::system(cmd.c_str());
    }
};

} // namespace sgdisk
} // namespace partitioner
} // namespace uli

#endif // ULI_PARTITIONER_SGDISK_WRAPPER_HPP
