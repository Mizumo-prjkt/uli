#ifndef ULI_PARTITIONER_DISKCHECK_HPP
#define ULI_PARTITIONER_DISKCHECK_HPP

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <array>
#include <memory>
#include <stdexcept>
#include <cstdio> // For popen/pclose
#include <cctype> // For isalnum
#include "os_check.hpp"
#include "../runtime/test_simulation.hpp"

namespace uli {
namespace partitioner {

struct DiskInfo {
    std::string name; // e.g., sda, nvme0n1
    std::string path; // e.g., /dev/sda
    std::string size; // e.g., 500G, 1T
    bool has_os;
    bool is_empty;
};

struct PartitionInfo {
    std::string name; // Local name, e.g., sda1, nvme0n1p1
    long long size_bytes; // Raw exact bytes
    std::string fs_type;
    std::string label;
    std::string mount_point;
};

class DiskCheck {
public:
    static std::string exec_command(const char* cmd) {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }

    // Fetches the total disk size in bytes via block enumeration
    static long long get_disk_size_bytes(const std::string& disk_path) {
        if (uli::runtime::TestSimulation::is_enabled()) {
            auto& cfg = uli::runtime::TestSimulation::get_config();
            long long size = 500LL * 1024 * 1024 * 1024; // Default 500 GiB
            size_t g_pos = cfg.disk_size.find("G");
            size_t m_pos = cfg.disk_size.find("M");
            if (g_pos != std::string::npos) {
                size = std::stoll(cfg.disk_size.substr(0, g_pos)) * 1024 * 1024 * 1024;
            } else if (m_pos != std::string::npos) {
                size = std::stoll(cfg.disk_size.substr(0, m_pos)) * 1024 * 1024;
            }
            return size;
        }

        std::string cmd = "lsblk -b -n -d -o SIZE " + disk_path;
        std::string output = exec_command(cmd.c_str());
        if (output.empty()) return 0;
        try {
            return std::stoll(output);
        } catch (...) {
            return 0;
        }
    }

    // Fetches exactly structured sub-partitions corresponding to the parent device
    static std::vector<PartitionInfo> get_disk_partitions(const std::string& disk_path) {
        if (uli::runtime::TestSimulation::is_enabled()) {
            return {};
        }

        std::vector<PartitionInfo> parts;
        std::string cmd = "lsblk -b -n -P -o NAME,SIZE,TYPE,FSTYPE,LABEL " + disk_path;
        std::string output = exec_command(cmd.c_str());
        std::stringstream ss(output);
        std::string line;
        
        while (std::getline(ss, line)) {
            if (line.find("TYPE=\"part\"") == std::string::npos) continue;
            
            auto get_val = [&](const std::string& key) {
                size_t pos = line.find(key + "=\"");
                if (pos == std::string::npos) return std::string("");
                pos += key.length() + 2;
                size_t end = line.find("\"", pos);
                if (end == std::string::npos) return std::string("");
                return line.substr(pos, end - pos);
            };
            
            PartitionInfo info;
            info.name = get_val("NAME");
            std::string sz = get_val("SIZE");
            info.size_bytes = sz.empty() ? 0 : std::stoll(sz);
            info.fs_type = get_val("FSTYPE");
            info.label = get_val("LABEL");
            
            if (info.fs_type.empty()) info.fs_type = "None";
            if (info.label.empty()) info.label = "None";
            
            if (info.size_bytes == 0) {
                info.label = "problematic partition";
            }
            
            parts.push_back(info);
        }
        
        return parts;
    }

    // Scans the system for all block devices
    static std::vector<DiskInfo> scan_disks() {
        if (uli::runtime::TestSimulation::is_enabled()) {
            std::vector<DiskInfo> fake_disks;
            DiskInfo info;
            auto& cfg = uli::runtime::TestSimulation::get_config();
            info.name = cfg.disk_type == "nvme" ? "nvme0n1" : "sda";
            info.path = "/dev/" + info.name;
            info.size = cfg.disk_size.empty() ? "500G" : cfg.disk_size;
            info.is_empty = true;
            info.has_os = false;
            fake_disks.push_back(info);
            return fake_disks;
        }

        std::vector<DiskInfo> disks;
        
        // lsblk output format: NAME,SIZE,TYPE (we want to filter for TYPE=disk)
        std::string cmd = "lsblk -d -n -o NAME,SIZE,TYPE | awk '$3==\"disk\"{print $1, $2}'";
        std::string output;
        
        try {
            output = exec_command(cmd.c_str());
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to execute lsblk for disk scanning." << std::endl;
            return disks;
        }

        std::stringstream ss(output);
        std::string name, size;
        
        while (ss >> name >> size) {
            // Avoid loops, ramdisks, cdroms
            if (name.find("loop") != std::string::npos || name.find("sr") != std::string::npos || name.find("ram") != std::string::npos) {
                continue;
            }

            DiskInfo info;
            info.name = name;
            info.path = "/dev/" + name;
            info.size = size;

            // Check if the disk has any existing partitions (proxy for emptiness)
            std::string part_cmd = "lsblk -n -o TYPE " + info.path + " | grep -q 'part'";
            int part_status = system(part_cmd.c_str());
            info.is_empty = (part_status != 0); // Active partitions mean it's not empty
            
            info.has_os = OSCheck::has_existing_os(info.path);
            
            disks.push_back(info);
        }

        return disks;
    }

    // High level target selection prioritizing config file -> empty disk -> largest safe disk
    static std::string select_target_disk(const std::string& intended_target) {
        auto disks = scan_disks();

        // 1. Direct Target Attempt
        if (!intended_target.empty()) {
            for (const auto& disk : disks) {
                if (disk.path == intended_target || disk.name == intended_target) {
                    if (disk.has_os) {
                        std::cerr << "[Warning] Intended target " << intended_target << " contains an existing OS operation system. Proceeding manually." << std::endl;
                    }
                    return disk.path;
                }
            }
            std::cout << "[DiskCheck] Intended target " << intended_target << " not found. Attempting fallback analysis..." << std::endl;
        }

        // 2. Fallback: Search for completely untouched/empty mapped drives
        for (const auto& disk : disks) {
            if (disk.is_empty && !disk.has_os) {
                std::cout << "[DiskCheck] Falling back to available empty unpartitioned disk: " << disk.path << " (" << disk.size << ")" << std::endl;
                return disk.path;
            }
        }

        // 3. Last Resort Fallback: Scan for the largest partitionable space that DOES NOT throw an OSCheck boundary error
        // Note: Accurately calculating bytes natively across partitions requires additional JSON parsing of lsblk bytes.
        // For simplicity, we just find the first available disk without a detected OS footprint.
        for (const auto& disk : disks) {
             if (!disk.has_os) {
                 std::cout << "[DiskCheck] Falling back to safe available disk lacking active OS footprints: " << disk.path << " (" << disk.size << ")" << std::endl;
                 return disk.path;
             }
        }

        std::cerr << "[Error] Could not locate a safe target disk for installation. All scanned block devices contain existing OS signatures or are protected." << std::endl;
        return "";
    }
};

} // namespace partitioner
} // namespace uli

#endif // ULI_PARTITIONER_DISKCHECK_HPP
