#ifndef ULI_RUNTIME_HANDLER_HPP
#define ULI_RUNTIME_HANDLER_HPP

#include <string>
#include <iostream>
#include <vector>
#include "dialogbox.hpp"
#include "warn.hpp"
#include "network_metrics.hpp"
#include "package_index.hpp"
#include "ui_dualpane_select.hpp"
#include "../package_mgr/dpkg_apt/dpkg_apt.hpp"
#include "../package_mgr/alps/alps_mgr.hpp"
#include "../package_mgr/apk/apk_mgr.hpp"
#include <memory>
// Bring in the partitioner abstract layer
#include "../partitioner/diskcheck.hpp"
#include "../partitioner/partdisk/partdisk.hpp"

#include "contents/drive/graph.hpp"
#include "contents/translations/lang_export.hpp"
#include "manual_part.hpp"
#include "../partitioner/sgdisk/sgdisk_wrapper.hpp"
#include "../partitioner/format/mkfs_wrapper.hpp"
#include "../partitioner/format/mount_wrapper.hpp"
#include "../partitioner/identifiers/blkid_wrapper.hpp"

namespace uli {
namespace runtime {

class UIHandler {
public:
    // Execute Disk Selection returning string target
    static std::string handle_disk_configuration(MenuState& state) {
        while (true) {
            Warn::print_info("Analyzing physical storage devices...");
            std::vector<uli::partitioner::DiskInfo> disks = uli::partitioner::DiskCheck::scan_disks();
            
            if (disks.empty()) {
                Warn::print_error("No partitionable block devices found.");
                return "";
            }

            std::vector<std::string> disk_choices;
            for (const auto& disk : disks) {
                std::string label = disk.path + " [" + disk.size + "]";
                if (disk.size == "0B" || disk.size == "0") {
                    label += " (Problematic partition)";
                } else if (disk.has_os) {
                    label += " (OS Detected - DANGER)";
                }
                disk_choices.push_back(label);
            }
            disk_choices.push_back(_tr("Back to Main Menu"));

            int sel = DialogBox::ask_selection(_tr("Storage Configuration"), _tr("Select an installation target:"), disk_choices);
            if (sel == -1 || sel == (int)disk_choices.size() - 1) return "";

            if (disks[sel].size == "0B" || disks[sel].size == "0") {
                DialogBox::show_alert(_tr("Error"), _tr("This is a problematic block partition (0B) and cannot be selected or edited."), [](){ return ""; });
                continue;
            }

            std::string selected_disk = disks[sel].path;

            bool back_to_disks = false;
            while (!back_to_disks) {
                if (state.drive == selected_disk && !state.partitions.empty()) {
                    auto ex_banner_func = [&state, selected_disk]() {
                        return ManualPartitionWizard::get_dynamic_layout_banner(selected_disk, state);
                    };
                    std::vector<std::string> ex_choices = {
                        _tr("Keep current layout and return to menu"),
                        _tr("Modify layout manually"),
                        _tr("Clear layout and start over"),
                        _tr("Cancel / Back to Disk Selection")
                    };
                    int ex_sel = DialogBox::ask_selection(_tr("Existing Layout Found"), _tr("You have already configured partitions for this drive."), ex_choices, ex_banner_func);
                    
                    if (ex_sel == 0) return selected_disk;
                    if (ex_sel == 1) { 
                        ManualPartitionWizard::start(selected_disk, state, true);
                        return selected_disk;
                    }
                    if (ex_sel == 2) {
                        state.partitions.clear();
                    } else {
                        back_to_disks = true;
                        continue; 
                    }
                }

                auto banner_func = [selected_disk]() {
                    return uli::runtime::contents::drive::DiskGraph::get_disk_layout_string(selected_disk);
                };

                std::vector<std::string> part_choices = {
                    _tr("Guided Installation (Purge Target Destructively)"),
                    _tr("Guided Installation (Harvest Available Free Space)"),
                    _tr("Manual Partitioning (Custom blocks & formats)"),
                    _tr("Cancel / Back to Disk Selection")
                };

                int part_sel = DialogBox::ask_selection(_tr("Partitioning"), _tr("Select partitioning method for ") + selected_disk, part_choices, banner_func);
                if (part_sel == 3 || part_sel == -1) {
                    back_to_disks = true;
                    continue;
                }

                if (part_sel == 0 || part_sel == 1) {
                    bool purge_intent = (part_sel == 0);
                    
                    std::vector<std::string> strategy_choices = {
                        _tr("Intelligent auto-compute (Determined by disk size)"),
                        _tr("Unified root partition (Everything in /)"),
                        _tr("Separate /home partition"),
                        _tr("Separate /var and /home partitions"),
                        _tr("Separate /opt, /var, and /home partitions"),
                        _tr("Cancel / Back")
                    };
                    int strat_sel = DialogBox::ask_selection(_tr("Guided Strategy"), _tr("Select division strategy:"), strategy_choices, banner_func);
                    if (strat_sel == 5 || strat_sel == -1) {
                        continue;
                    }
                    
                    MenuState temp_state = state;
                    temp_state.purge_disk_intent = purge_intent;
                    temp_state.partitions.clear();
                    
                    long long total_disk = uli::partitioner::DiskCheck::get_disk_size_bytes(selected_disk);
                    long long used_bytes = 0;
                    
                    if (!temp_state.purge_disk_intent) { 
                        auto existing_parts = uli::partitioner::DiskCheck::get_disk_partitions(selected_disk);
                        int pnum = 1;
                        for (const auto& ep : existing_parts) {
                            PartitionConfig cfg;
                            cfg.device_path = "/dev/" + ep.name;
                            cfg.fs_type = ep.fs_type;
                            cfg.label = ep.label;
                            cfg.part_num = pnum++;
                            cfg.is_deferred = false;
                            cfg.size_cmd = std::to_string(ep.size_bytes);
                            temp_state.partitions.push_back(cfg);
                            used_bytes += ep.size_bytes;
                        }
                    }
                    
                    long long available_bytes = total_disk - used_bytes - (2 * 1024 * 1024); // 2MB padding
                    long long absolute_min = 24LL * 1024 * 1024 * 1024; // 24GB
                    
                    if (available_bytes < absolute_min) {
                        DialogBox::show_alert(_tr("Error"), _tr("Intelligent Layout Failed: Insufficient Free Space (Requires 24GB Minimum)."), banner_func);
                        continue; 
                    }
                    
                    int pnum = temp_state.partitions.size() + 1;
                    
                    PartitionConfig boot_p;
                    boot_p.part_num = pnum++;
                    boot_p.fs_type = "vfat";
                    boot_p.mount_point = "/boot";
                    boot_p.type_code = "ef00";
                    boot_p.size_cmd = "+1G";
                    boot_p.is_deferred = true;
                    temp_state.partitions.push_back(boot_p);
                    
                    available_bytes -= (1LL * 1024 * 1024 * 1024);
                    
                    long long gb14 = 14LL * 1024 * 1024 * 1024;
                    long long gb150 = 150LL * 1024 * 1024 * 1024;
                    
                    int logic_path = 0; // 0=Unified, 1=Root+Home, 2=Root+Var+Home, 3=Root+Opt+Var+Home
                    if (strat_sel == 0) { // Auto-compute
                        if (available_bytes < gb14) logic_path = 0;
                        else if (available_bytes < gb150) logic_path = 1;
                        else logic_path = 2;
                    } else if (strat_sel == 1) logic_path = 0;
                    else if (strat_sel == 2) logic_path = 1;
                    else if (strat_sel == 3) logic_path = 2;
                    else if (strat_sel == 4) logic_path = 3;

                    // Swap Acknowledgment / Request
                    bool include_swap = false;
                    if (available_bytes > 10LL * 1024 * 1024 * 1024) { // Only suggest if > 10GB
                        std::vector<std::string> swap_choices = {_tr("Include 4GiB Swap Partition"), _tr("No Swap Partition (Acknowledge)")};
                        int swap_sel = DialogBox::ask_selection(_tr("Swap Configuration"), _tr("Do you want to include a swap partition?"), swap_choices, banner_func);
                        if (swap_sel == 0) include_swap = true;
                        else if (swap_sel == -1) continue; 
                    }
                    
                    if (include_swap) {
                        PartitionConfig swap_p;
                        swap_p.part_num = pnum++;
                        swap_p.fs_type = "swap";
                        swap_p.mount_point = ""; // Swap has no mount point string
                        swap_p.type_code = "8200";
                        swap_p.size_cmd = "+4G";
                        swap_p.is_deferred = true;
                        temp_state.partitions.push_back(swap_p);
                        available_bytes -= (4LL * 1024 * 1024 * 1024);
                    }

                    if (logic_path == 0) { // Unified
                        PartitionConfig root_p;
                        root_p.part_num = pnum++;
                        root_p.fs_type = "ext4";
                        root_p.mount_point = "/";
                        root_p.type_code = "8304"; // x86-64 root
                        root_p.size_cmd = "0"; 
                        root_p.is_deferred = true;
                        temp_state.partitions.push_back(root_p);
                    } else if (logic_path == 1) { // Root + Home
                        PartitionConfig root_p;
                        root_p.part_num = pnum++;
                        root_p.fs_type = "ext4";
                        root_p.mount_point = "/";
                        root_p.type_code = "8304";
                        root_p.size_cmd = "+40G"; 
                        root_p.is_deferred = true;
                        temp_state.partitions.push_back(root_p);
                        
                        PartitionConfig home_p;
                        home_p.part_num = pnum++;
                        home_p.fs_type = "ext4";
                        home_p.mount_point = "/home";
                        home_p.type_code = "8302"; // x86-64 home
                        home_p.size_cmd = "0"; 
                        home_p.is_deferred = true;
                        temp_state.partitions.push_back(home_p);
                    } else if (logic_path == 2) { // Root + Var + Home
                        PartitionConfig root_p;
                        root_p.part_num = pnum++;
                        root_p.fs_type = "ext4";
                        root_p.mount_point = "/";
                        root_p.type_code = "8304";
                        root_p.size_cmd = "+40G"; 
                        root_p.is_deferred = true;
                        temp_state.partitions.push_back(root_p);
                        
                        PartitionConfig var_p;
                        var_p.part_num = pnum++;
                        var_p.fs_type = "ext4";
                        var_p.mount_point = "/var";
                        var_p.type_code = "8300";
                        var_p.size_cmd = "+20G"; 
                        var_p.is_deferred = true;
                        temp_state.partitions.push_back(var_p);
                        
                        PartitionConfig home_p;
                        home_p.part_num = pnum++;
                        home_p.fs_type = "ext4";
                        home_p.mount_point = "/home";
                        home_p.type_code = "8302";
                        home_p.size_cmd = "0"; 
                        home_p.is_deferred = true;
                        temp_state.partitions.push_back(home_p);
                    } else if (logic_path == 3) { // Root + Opt + Var + Home
                        PartitionConfig root_p;
                        root_p.part_num = pnum++;
                        root_p.fs_type = "ext4";
                        root_p.mount_point = "/";
                        root_p.type_code = "8304";
                        root_p.size_cmd = "+40G"; 
                        root_p.is_deferred = true;
                        temp_state.partitions.push_back(root_p);
                        
                        PartitionConfig opt_p;
                        opt_p.part_num = pnum++;
                        opt_p.fs_type = "ext4";
                        opt_p.mount_point = "/opt";
                        opt_p.type_code = "8300";
                        opt_p.size_cmd = "+20G"; 
                        opt_p.is_deferred = true;
                        temp_state.partitions.push_back(opt_p);
                        
                         PartitionConfig var_p;
                        var_p.part_num = pnum++;
                        var_p.fs_type = "ext4";
                        var_p.mount_point = "/var";
                        var_p.type_code = "8300";
                        var_p.size_cmd = "+20G"; 
                        var_p.is_deferred = true;
                        temp_state.partitions.push_back(var_p);
                        
                        PartitionConfig home_p;
                        home_p.part_num = pnum++;
                        home_p.fs_type = "ext4";
                        home_p.mount_point = "/home";
                        home_p.type_code = "8302";
                        home_p.size_cmd = "0"; 
                        home_p.is_deferred = true;
                        temp_state.partitions.push_back(home_p);
                    }
                    
                    auto preview_banner = [&temp_state, selected_disk]() {
                        std::string banner = ManualPartitionWizard::get_dynamic_layout_banner(selected_disk, temp_state);
                        bool has_swap = false;
                        for (const auto& p : temp_state.partitions) if (p.fs_type == "swap") has_swap = true;
                        if (!has_swap) {
                            banner += "\n\n" + std::string(DesignUI::YELLOW) + _tr("!! WARNING: No Swap partition configured. !!") + DesignUI::RESET;
                        }
                        return banner;
                    };
                    
                    std::vector<std::string> confirm_choices = {_tr("Yes, Apply Layout"), _tr("No, Cancel")};
                    int confirm_sel = DialogBox::ask_selection(_tr("Layout Preview"), _tr("Apply this layout to the disk?"), confirm_choices, preview_banner);
                    if (confirm_sel == 0) {
                        state = temp_state;
                        ManualPartitionWizard::start(selected_disk, state, true);
                        return selected_disk;
                    } else {
                        continue;
                    }
                } else if (part_sel == 2) {
                    ManualPartitionWizard::start(selected_disk, state);
                    return selected_disk;
                }
            }
        }
    }

    // Process all pending deferred partition configurations globally before install unpacks
    static bool execute_deferred_partitions(MenuState& state) {
        if (state.partitions.empty() || state.drive.empty()) return true;

        bool has_deferred = false;
        for (const auto& p : state.partitions) if (p.is_deferred) has_deferred = true;
        if (!has_deferred) return true;

        Warn::print_info("Executing deferred partition layouts onto " + state.drive + "...");
        std::cout << "[mkfs] Initializing new GUID Partition Table...\n";
        uli::partitioner::sgdisk::SgdiskWrapper::create_gpt_table(state.drive);

        for (auto& p : state.partitions) {
            if (!p.is_deferred) continue;
            
            std::cout << "[mkfs] Slicing partition " << p.part_num << " (" << p.size_cmd << ")...\n";
            bool success = uli::partitioner::sgdisk::SgdiskWrapper::create_partition(state.drive, p.part_num, "0", p.size_cmd, p.type_code);
            if (!success) {
                Warn::print_error("Failed to construct partition " + std::to_string(p.part_num));
                return false;
            }
            
            uli::partitioner::sgdisk::SgdiskWrapper::refresh_part_table(state.drive);
            std::system("sleep 1"); // udev map refresh
            
            std::string dev_part_path = state.drive;
            if (state.drive.back() >= '0' && state.drive.back() <= '9') dev_part_path += "p" + std::to_string(p.part_num);
            else dev_part_path += std::to_string(p.part_num);
            
            p.device_path = dev_part_path;
            
            if (!uli::partitioner::format::MkfsWrapper::format_partition(dev_part_path, p.fs_type, p.label)) {
                Warn::print_error("Failed to format filesystem on " + dev_part_path);
                return false;
            }

            std::system("sleep 1"); // Generate BLKID targets
            p.partuuid = uli::partitioner::identifiers::BlkidWrapper::get_partuuid(dev_part_path);
            p.is_deferred = false;
            std::cout << "[mkfs] " << dev_part_path << " cleanly formatted. PARTUUID=" << p.partuuid << "\n";
        }
        return true;
    }

    // Mounts all partitions in the correct order specifically for Arch Linux
    static bool mount_all_partitions(MenuState& state, const std::string& distro) {
        if (distro != "Arch Linux") return true;

        // 1. Initial Cleanup: Ensure /mnt is fresh and free
        uli::partitioner::format::MountWrapper::swapoff_all();
        uli::partitioner::format::MountWrapper::umount_recursive("/mnt");

        // Sort partitions: Swap first, then Root, then others by path depth
        std::vector<PartitionConfig> sorted_parts = state.partitions;
        std::sort(sorted_parts.begin(), sorted_parts.end(), [](const PartitionConfig& a, const PartitionConfig& b) {
            if (a.fs_type == "swap" && b.fs_type != "swap") return true;
            if (a.fs_type != "swap" && b.fs_type == "swap") return false;
            if (a.mount_point == "/" && b.mount_point != "/") return true;
            if (a.mount_point != "/" && b.mount_point == "/") return false;
            return a.mount_point.length() < b.mount_point.length();
        });

        for (const auto& p : sorted_parts) {
            if (p.device_path.empty()) continue;

            if (p.fs_type == "swap") {
                if (!uli::partitioner::format::MountWrapper::swapon(p.device_path)) {
                    Warn::print_error("Failed to initialize swap on " + p.device_path);
                    return false;
                }
            } else if (p.mount_point == "/") {
                if (!uli::partitioner::format::MountWrapper::mount(p.device_path, "/mnt")) {
                    Warn::print_error("Failed to mount Root partition to /mnt");
                    return false;
                }
            } else if (!p.mount_point.empty()) {
                std::string target = "/mnt" + p.mount_point;
                if (!uli::partitioner::format::MountWrapper::mount(p.device_path, target)) {
                    Warn::print_error("Failed to mount " + p.mount_point + " to " + target);
                    return false;
                }
            }
        }
        return true;
    }

    // Safely unmounts everything to prevent corrupted mount states on failure
    static void cleanup_mounts(MenuState& state, const std::string& distro) {
        if (distro != "Arch Linux") return;
        uli::partitioner::format::MountWrapper::swapoff_all();
        uli::partitioner::format::MountWrapper::umount_recursive("/mnt");
    }

    // Ensures essential packages are present for a bootable Arch system
    static std::vector<std::string> refine_package_list(const std::string& distro, const MenuState& state) {
        std::vector<std::string> final_list = state.additional_packages;
        if (distro == "Arch Linux") {
            auto ensure_pkg = [&](const std::string& pkg) {
                if (std::find(final_list.begin(), final_list.end(), pkg) == final_list.end()) {
                    final_list.push_back(pkg);
                }
            };
            ensure_pkg("base");
            ensure_pkg("linux-firmware");
            
            // Kernel is already in state.kernel (default "linux"), also ensure headers if requested
            if (!state.kernel.empty()) ensure_pkg(state.kernel);
        }
        return final_list;
    }

    // Generates fstab for the target system using genfstab (Arch Specific)
    static bool generate_fstab(const std::string& mount_point) {
        std::cout << "[fstab] Generating /etc/fstab for target " << mount_point << "..." << std::endl;
        std::string cmd = "genfstab -U " + mount_point + " >> " + mount_point + "/etc/fstab 2>/dev/null";
        return (std::system(cmd.c_str()) == 0);
    }

    // Wrap the package lookup cache and return selected
    static std::vector<std::string> handle_package_search(const std::string& active_distro, std::vector<std::string> current_pkgs, const MenuState& state) {
        std::unique_ptr<uli::package_mgr::PackageManagerInterface> pm;
        if (active_distro == "Debian" || active_distro == "Ubuntu") {
            pm = std::make_unique<uli::package_mgr::DpkgAptManager>();
        } else if (active_distro == "Arch Linux") {
            pm = std::make_unique<uli::package_mgr::alps::AlpsManager>();
        } else if (active_distro == "Alpine Linux") {
            pm = std::make_unique<uli::package_mgr::apk::ApkManager>();
        } else {
            DialogBox::show_alert("Index Error", "Unsupported package manager for this distribution.");
            return current_pkgs;
        }

        // Failsafe Ping
        std::vector<std::string> test_ping = pm->search_packages(""); // empty query returns empty gracefully, let's explicitly ping 'nano' to test binary presence
        test_ping = pm->search_packages("nano");
        
        if (!test_ping.empty() && test_ping[0] == "_ULI_PM_ERROR_") {
            DialogBox::show_alert("Index Error", "The underlying package manager failed to communicate. Indexing packages will not work. Expect issues on the install run.");
            std::string man_input = DialogBox::ask_input("Manual Package Entry", "Search is offline. Type explicit packages separated by spaces.\n\nWARNING: Ensure strict spelling or else the install may fail at runtime! You are on your own.");
            if (!man_input.empty()) {
                std::istringstream iss(man_input);
                std::string pkg;
                while (iss >> pkg) {
                    if (std::find(current_pkgs.begin(), current_pkgs.end(), pkg) == current_pkgs.end()) {
                        current_pkgs.push_back(pkg);
                    }
                }
            }
            return current_pkgs;
        }

        return UIDualPaneSelect::show_package_search(pm.get(), current_pkgs, state.manual_mappings);
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_HANDLER_HPP
