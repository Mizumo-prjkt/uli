#ifndef ULI_PARTITIONER_PARTITION_TRANSLATOR_HPP
#define ULI_PARTITIONER_PARTITION_TRANSLATOR_HPP

#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <iomanip>
#include "diskcheck.hpp"

namespace uli {
namespace partitioner {

struct YamlPartition {
    std::string name;
    long long size_bytes; // Explicit size given in the profile (for calculating % factor)
    std::string fs_type;
    std::string mount_point;
};

struct LivePartition {
    std::string name;
    long long scaled_size_bytes; // Size applied proportionally
    std::string fs_type;
    std::string mount_point;
};

class PartitionTranslator {
public:
    // Minimum 14 GiB size threshold (14 * 1024^3 in bytes)
    static constexpr long long MIN_DISK_SIZE_BYTES = 14ULL * 1024 * 1024 * 1024;

    // Evaluates the profiled disk target against the physical host system.
    // Returns the exact mapping absolute path if found, or maps across interfaces (sda <-> nvme0n1).
    static std::string resolve_target_disk(const std::string& profiled_disk_name, const std::vector<DiskInfo>& live_disks) {
        // 1. Direct Target Match
        // E.g., if profile asks for sda and sda exists, ALWAYS follow it!
        for (const auto& disk : live_disks) {
            if (disk.name == profiled_disk_name || disk.path == profiled_disk_name) {
                return disk.path;
            }
        }

        // 2. Cross-Interface Fallback Equivalence
        // Deduce the rank of the target device
        int target_rank = -1;
        if (profiled_disk_name.substr(0, 2) == "sd" && profiled_disk_name.length() >= 3) {
            target_rank = profiled_disk_name[2] - 'a'; // sda = 0, sdb = 1
        } else if (profiled_disk_name.substr(0, 4) == "nvme" && profiled_disk_name.length() >= 5) {
            target_rank = profiled_disk_name[4] - '0'; // nvme0n1 = 0, nvme1n1 = 1
        }

        if (target_rank == -1) return "";

        // Collect grouped arrays natively
        std::vector<DiskInfo> live_sd;
        std::vector<DiskInfo> live_nvme;
        for (const auto& disk : live_disks) {
            if (disk.name.substr(0, 2) == "sd") live_sd.push_back(disk);
            else if (disk.name.substr(0, 4) == "nvme") live_nvme.push_back(disk);
        }

        auto get_by_rank = [](const std::vector<DiskInfo>& lst, int r) -> const DiskInfo* {
            for (const auto& d : lst) {
                if (d.name.substr(0, 2) == "sd" && (d.name[2] - 'a') == r) return &d;
                if (d.name.substr(0, 4) == "nvme" && (d.name[4] - '0') == r) return &d;
            }
            return nullptr;
        };

        if (profiled_disk_name.substr(0, 2) == "sd") {
            const DiskInfo* eq = get_by_rank(live_nvme, target_rank);
            if (eq) {
                std::cout << "[DiskTranslator] Resolved " << profiled_disk_name << " -> equivalent interface -> " << eq->path << std::endl;
                return eq->path;
            }
        } else if (profiled_disk_name.substr(0, 4) == "nvme") {
            const DiskInfo* eq = get_by_rank(live_sd, target_rank);
            if (eq) {
                std::cout << "[DiskTranslator] Resolved " << profiled_disk_name << " -> equivalent interface -> " << eq->path << std::endl;
                return eq->path;
            }
        }

        return "";
    }

    // Examines disk thresholds and converts exact profiled partition layouts into % scaled proportions internally.
    static std::vector<LivePartition> evaluate_and_scale(
        const std::string& live_disk_path, 
        long long profile_disk_total_size, 
        const std::vector<YamlPartition>& profile_partitions,
        bool force_small_disk) 
    {
        long long live_disk_size = DiskCheck::get_disk_size_bytes(live_disk_path);
        if (live_disk_size == 0) {
            throw std::runtime_error("Could not determine live disk size for scaling: " + live_disk_path);
        }

        // 14 GiB Threshold Protection Check
        if (live_disk_size < MIN_DISK_SIZE_BYTES) {
            if (!force_small_disk) {
                std::cerr << "\n[!] PARTITIONER ABORT: Target disk " << live_disk_path 
                          << " failed Minimum 14 GiB threshold size constraint." << std::endl;
                std::cerr << "Provide a valid target drive, partition manually, or bypass this using the --force-small-disk attribute.\n" << std::endl;
                throw std::runtime_error("Disk Threshold Constraint Failed");
            } else {
                std::cout << "\n[!] WARNING: Minimum disk threshold 14 GiB dynamically bypassed by Flag constraint! Unpredictable mapping allocations may happen on " << live_disk_path << ".\n" << std::endl;
            }
        }

        std::vector<LivePartition> mapped;
        for (const auto& y_part : profile_partitions) {
            // Factor formulation: percentage of YAML total disk assumed
            double percentage_factor = static_cast<double>(y_part.size_bytes) / static_cast<double>(profile_disk_total_size);
            
            // Translate chunk out of LIVE system bytes size using identical percentage constraint
            long long targeted_bytes = static_cast<long long>(percentage_factor * live_disk_size);
            
            LivePartition lp;
            lp.name = y_part.name;
            lp.fs_type = y_part.fs_type;
            lp.mount_point = y_part.mount_point;
            lp.scaled_size_bytes = targeted_bytes;
            
            mapped.push_back(lp);
        }

        return mapped;
    }
};

} // namespace partitioner
} // namespace uli

#endif // ULI_PARTITIONER_PARTITION_TRANSLATOR_HPP
