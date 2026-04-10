#ifndef ULI_PARTITIONER_SGDISK_WRAPPER_HPP
#define ULI_PARTITIONER_SGDISK_WRAPPER_HPP

#include <string>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include "../../runtime/blackbox.hpp"
#include <unistd.h>

namespace uli {
namespace partitioner {
namespace sgdisk {

enum class PartitionTool { SGDISK, FDISK, PARTED, UNKNOWN };

struct PartitionRange {
    long long start;
    long long end;
};

class PartitionComputeLayer {
public:
    // Standard 1MiB alignment = 2048 sectors (on 512B drives) or 256 sectors (on 4K drives)
    static long long align_to_1mib(long long sector, int sector_size) {
        long long sectors_per_mib = (1024LL * 1024LL) / sector_size;
        return ((sector + sectors_per_mib - 1) / sectors_per_mib) * sectors_per_mib;
    }

    static long long parse_size_to_sectors(const std::string& size_str, int sector_size) {
        if (size_str == "0" || size_str.empty()) return 0;
        
        long long bytes = 0;
        char unit = size_str.back();
        std::string val = size_str.substr(1, size_str.size() - 2); // skip + and unit
        
        try {
            long long num = std::stoll(val);
            if (unit == 'M' || unit == 'm') bytes = num * 1024LL * 1024LL;
            else if (unit == 'G' || unit == 'g') bytes = num * 1024LL * 1024LL * 1024LL;
            else if (unit == 'K' || unit == 'k') bytes = num * 1024LL;
        } catch (...) { return 0; }

        return bytes / sector_size;
    }
};

class SgdiskWrapper {
private:
    static PartitionTool active_tool;
    static long long next_free_sector;
    static int detected_sector_size;

    static bool has_bin(const std::string& name) {
        std::string cmd = "command -v " + name + " > /dev/null 2>&1";
        return (std::system(cmd.c_str()) == 0);
    }

    static std::string translate_type(const std::string& sg_code, PartitionTool tool) {
        if (tool == PartitionTool::SGDISK) return sg_code;
        if (tool == PartitionTool::FDISK) {
            if (sg_code == "ef00") return "1";  // EFI System
            if (sg_code == "8200") return "19"; // Linux swap
            if (sg_code == "8300") return "20"; // Linux filesystem
            if (sg_code == "8304") return "21"; // Linux root (x86-64)
            if (sg_code == "8302") return "23"; // Linux home
            return "20"; // Default Linux filesystem
        }
        if (tool == PartitionTool::PARTED) {
            if (sg_code == "ef00") return "esp";
            return ""; 
        }
        return "";
    }

public:
    static bool create_gpt_table(const std::string& disk_path) {
        uli::runtime::BlackBox::log("ENTER create_gpt_table: " + disk_path);
        next_free_sector = 2048; // Start at 1MiB for alignment
        detected_sector_size = 512; // Default, could be detected via blockdev --getss

        // Level 1: sgdisk
        if (has_bin("sgdisk")) {
            active_tool = PartitionTool::SGDISK;
            std::string cmd = "sgdisk --zap-all --clear --mbrtogpt \"" + disk_path + "\" > /dev/null 2>&1";
            if (std::system(cmd.c_str()) == 0) return true;
        }

        // Level 2: fdisk
        if (has_bin("fdisk")) {
            active_tool = PartitionTool::FDISK;
            std::cout << "[partitioner] Fallback: Wiping disk signature...\n";
            std::string wipe = "dd if=/dev/zero of=\"" + disk_path + "\" bs=1M count=10 conv=notrunc > /dev/null 2>&1";
            std::system(wipe.c_str());
            std::string cmd = "printf \"g\\nw\\n\" | fdisk \"" + disk_path + "\" > /dev/null 2>&1";
            if (std::system(cmd.c_str()) == 0) return true;
        }

        // Level 3: parted
        if (has_bin("parted")) {
            active_tool = PartitionTool::PARTED;
            std::string cmd = "parted -s \"" + disk_path + "\" mklabel gpt > /dev/null 2>&1";
            if (std::system(cmd.c_str()) == 0) return true;
        }

        active_tool = PartitionTool::UNKNOWN;
        return false;
    }

    static bool create_partition(const std::string& disk_path, int part_num, const std::string& size_start, const std::string& size_end, const std::string& type_code) {
        long long start = next_free_sector;
        long long count = PartitionComputeLayer::parse_size_to_sectors(size_end, detected_sector_size);
        long long end = (count == 0) ? 0 : (start + count - 1);

        std::string type = translate_type(type_code, active_tool);
        bool success = false;

        if (active_tool == PartitionTool::SGDISK) {
            std::string cmd = "sgdisk -n " + std::to_string(part_num) + ":" + std::to_string(start) + ":" + (end == 0 ? "" : std::to_string(end))
                            + " -t " + std::to_string(part_num) + ":" + type + " \"" + disk_path + "\" > /dev/null 2>&1";
            success = (std::system(cmd.c_str()) == 0);
        } else if (active_tool == PartitionTool::FDISK) {
            // Scripted fdisk: n (new), num, start, end, t (type), num, type, w (write)
            std::string s_start = std::to_string(start);
            std::string s_end = (end == 0) ? "" : ("+" + size_end.substr(1));
            std::string cmd = "printf \"n\\n" + std::to_string(part_num) + "\\n" + s_start + "\\n" + s_end + "\\nt\\n" + std::to_string(part_num) + "\\n" + type + "\\nw\\n\" | fdisk \"" + disk_path + "\" > /dev/null 2>&1";
            success = (std::system(cmd.c_str()) == 0);
        } else if (active_tool == PartitionTool::PARTED) {
            std::string s_start = std::to_string(start * detected_sector_size / 1024) + "KiB";
            std::string s_end = (end == 0) ? "100%" : std::to_string((end + 1) * detected_sector_size / 1024) + "KiB";
            std::string cmd = "parted -s \"" + disk_path + "\" mkpart primary " + s_start + " " + s_end + " > /dev/null 2>&1";
            success = (std::system(cmd.c_str()) == 0);
        }

        if (success) {
            if (count > 0) next_free_sector = PartitionComputeLayer::align_to_1mib(end + 1, detected_sector_size);
            return true;
        }
        return false;
    }

    static void refresh_part_table(const std::string& disk_path) {
        std::system(("partprobe \"" + disk_path + "\" > /dev/null 2>&1").c_str());
        std::system("udevadm settle");
    }
};

// Initialize static members
inline PartitionTool SgdiskWrapper::active_tool = PartitionTool::UNKNOWN;
inline long long SgdiskWrapper::next_free_sector = 2048;
inline int SgdiskWrapper::detected_sector_size = 512;

} // namespace sgdisk
} // namespace partitioner
} // namespace uli

#endif // ULI_PARTITIONER_SGDISK_WRAPPER_HPP
