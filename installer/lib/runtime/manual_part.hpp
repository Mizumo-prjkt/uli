#ifndef ULI_RUNTIME_MANUAL_PART_HPP
#define ULI_RUNTIME_MANUAL_PART_HPP

#include "../partitioner/format/mkfs_wrapper.hpp"
#include "../partitioner/identifiers/blkid_wrapper.hpp"
#include "../partitioner/identifiers/flag_partitions.hpp"
#include "../partitioner/sgdisk/sgdisk_wrapper.hpp"
#include "contents/drive/graph.hpp"
#include "dialogbox.hpp"
#include "inputbox.hpp"
#include "menu.hpp" // Must come early
#include "contents/translations/lang_export.hpp"
#include <string>
#include <vector>

namespace uli {
namespace runtime {

class ManualPartitionWizard {
public:
  static long long parse_sz_cmd_to_bytes(const std::string &cmd) {
    if (cmd == "0" || cmd == "+0" || cmd.empty())
      return 0; // Remainder
    long long val = 0;
    try {
      val = std::stoll(cmd);
    } catch (...) {
      return 0;
    }
    if (cmd.back() == 'M' || cmd.back() == 'm')
      val *= 1024 * 1024;
    else if (cmd.back() == 'G' || cmd.back() == 'g')
      val *= 1024 * 1024 * 1024;
    return val;
  }

  static bool validate_capacity_bounds(const std::string &disk_path,
                                       const MenuState &state,
                                       long long requested_bytes) {
    long long total_disk =
        uli::partitioner::DiskCheck::get_disk_size_bytes(disk_path);
    long long used_bytes = 0;
    for (const auto &p : state.partitions) {
      used_bytes += parse_sz_cmd_to_bytes(p.size_cmd);
    }
    long long magic_padding = 2 * 1024 * 1024; // 2MB Failsafe alignment padding
    return (used_bytes + requested_bytes + magic_padding) <= total_disk;
  }

  static std::string get_dynamic_layout_banner(const std::string &disk_path,
                                               const MenuState &state) {
    if (state.partitions.empty() && !state.purge_disk_intent) {
      return uli::runtime::contents::drive::DiskGraph::get_disk_layout_string(
          disk_path);
    }
    long long total_bytes =
        uli::partitioner::DiskCheck::get_disk_size_bytes(disk_path);
    long long used_bytes = 0;
    long long padding = 2 * 1024 * 1024;
    for (const auto &p : state.partitions) {
      used_bytes += parse_sz_cmd_to_bytes(p.size_cmd);
    }
    std::vector<uli::partitioner::PartitionInfo> override_parts;
    for (const auto &p : state.partitions) {
      uli::partitioner::PartitionInfo pinfo;
      std::string base_name = disk_path;
      if (base_name.compare(0, 5, "/dev/") == 0)
        base_name = base_name.substr(5);
      if (base_name.back() >= '0' && base_name.back() <= '9')
        base_name += "p";
      pinfo.name = base_name + std::to_string(p.part_num);
      pinfo.fs_type = p.fs_type;
      pinfo.label = p.label;
      pinfo.mount_point = p.mount_point;
      long long sz = parse_sz_cmd_to_bytes(p.size_cmd);
      if (sz == 0) {
        sz = total_bytes - used_bytes - padding;
        if (sz < 0)
          sz = 0;
        used_bytes += sz;
      }
      pinfo.size_bytes = sz;
      override_parts.push_back(pinfo);
    }

    bool has_swap = false;
    for (const auto& p : state.partitions) if (p.fs_type == "swap") has_swap = true;

    std::string out = uli::runtime::contents::drive::DiskGraph::
        get_disk_layout_string_from_parts(disk_path, total_bytes,
                                          override_parts);
    
    if (!has_swap && !state.partitions.empty()) {
        out += "\n" + std::string(DesignUI::DIM) + _tr("(Note: No swap configured)") + DesignUI::RESET;
    }
    return out;
  }

  static void start(const std::string &disk_path, MenuState &state,
                    const std::string& os_distro, bool resume = false) {

    if (!resume) {
      std::vector<std::string> init_choices = {
          _tr("Purge Target Fully (Zap GPT & Formats)"),
          _tr("Continue without purging (Preserve existing partitions)"), _tr("Back")};
      int init_sel = DialogBox::ask_selection(
          _tr("Manual Partitioning"), _tr("Initialize new GUID Partition Table?"),
          init_choices);
      if (init_sel == 2 || init_sel == -1)
        return;

      state.purge_disk_intent = (init_sel == 0);
      state.partitions.clear();

      if (!state.purge_disk_intent) {
        auto existing_parts =
            uli::partitioner::DiskCheck::get_disk_partitions(disk_path);
        int pnum = 1;
        for (const auto &ep : existing_parts) {
          PartitionConfig cfg;
          cfg.device_path = "/dev/" + ep.name;
          cfg.fs_type = ep.fs_type;
          cfg.label = ep.label;
          cfg.part_num = pnum++;
          cfg.is_deferred = false;
          cfg.size_cmd = std::to_string(ep.size_bytes);
          state.partitions.push_back(cfg);
        }
      }
    }

    bool part_loop = true;
    int part_num = state.partitions.size() + 1;

    while (part_loop) {
      DesignUI::clear_screen();
      auto banner_func = [&disk_path, &state]() {
        return get_dynamic_layout_banner(disk_path, state);
      };

      std::vector<std::string> action_choices = {
          _tr("Create new deferred partition"), _tr("Modify existing partition block"),
          _tr("Finish and Save Layout")};

      int action_sel = DialogBox::ask_selection(_tr("Manual Partitioning"),
                                                _tr("Action on ") + disk_path,
                                                action_choices, banner_func);
      if (action_sel == 2)
        break; // Done
      if (action_sel == -1)
        continue; // Escaped

      if (action_sel == 1) { // Modify existing
        if (state.partitions.empty()) {
          DialogBox::show_alert(_tr("Error"), _tr("No partitions created yet to modify."),
                                banner_func);
          continue;
        }
        std::vector<std::string> mod_choices;
        for (size_t i = 0; i < state.partitions.size(); i++) {
          auto &p = state.partitions[i];
          
          std::string display_sz = p.size_cmd;
          if (p.size_cmd == "0" || p.size_cmd == "+0" || p.size_cmd.empty()) {
            display_sz = _tr("Remaining Space");
          } else {
            long long b_val = parse_sz_cmd_to_bytes(p.size_cmd);
            if (b_val > 0) {
              display_sz = uli::runtime::contents::drive::DiskGraph::format_bytes(b_val);
            }
          }
          
          std::string fs_label = p.fs_type.empty() ? _tr("Unformatted") : p.fs_type;
          
          mod_choices.push_back(
              _tr("Partition ") + std::to_string(p.part_num) + " (" + display_sz + ") [" + fs_label +
              "] -> " + (p.mount_point.empty() ? _tr("None") : p.mount_point));
        }
        int p_idx = DialogBox::ask_selection(
            _tr("Modify Partition"), _tr("Select partition to edit:"), mod_choices,
            banner_func);
        if (p_idx == -1)
          continue;

        auto &p = state.partitions[p_idx];
        
        if (p.label == "problematic partition") {
            DialogBox::show_alert(_tr("Error"), _tr("This is a problematic partition (0B) and cannot be modified or edited."), banner_func);
            continue;
        }
        
        bool mod_loop = true;
        while (mod_loop) {
          std::string display_sz = p.size_cmd;
          if (p.size_cmd == "0" || p.size_cmd == "+0" || p.size_cmd.empty()) {
            display_sz = _tr("Remaining Space");
          } else {
            long long b_val = parse_sz_cmd_to_bytes(p.size_cmd);
            if (b_val > 0) {
              display_sz =
                  uli::runtime::contents::drive::DiskGraph::format_bytes(b_val);
            }
          }

          std::vector<std::string> props = {
              _tr("Size bounds (Current: ") + display_sz + ")",
              _tr("Filesystem (Current: ") + p.fs_type + ")",
              _tr("Mount Point (Current: ") + p.mount_point + ")",
              _tr("Mount Options (Current: ") + p.mount_options + ")",
              _tr("Label (Current: ") + p.label + ")",
              _tr("Delete Partition"),
              _tr("Done modifying")};
          int prop_sel = DialogBox::ask_selection(
              _tr("Edit Partition ") + std::to_string(p.part_num),
              _tr("Select property to change:"), props, banner_func);

          if (prop_sel == 6 || prop_sel == -1)
            break;

          if (prop_sel == 0) { // Size
            std::vector<std::string> units = {"MB", "MiB", "GB", "GiB"};
            int unit_sel = DialogBox::ask_selection(
                _tr("Unit of Measurement"), _tr("Select measuring metric:"), units);
            if (unit_sel != -1) {
              std::string size_input = DialogBox::ask_input(
                  _tr("Partition Size"),
                  _tr("Enter partition size in ") + units[unit_sel] +
                      _tr(" (e.g. +512, or 0 for remaining space):"));
              if (!size_input.empty()) {
                std::string sgdisk_size = size_input;
                if (sgdisk_size != "0" && sgdisk_size != "+0") {
                  if (sgdisk_size[0] != '+')
                    sgdisk_size = "+" + sgdisk_size;
                  if (units[unit_sel] == "MB" || units[unit_sel] == "MiB")
                    sgdisk_size += "M";
                  else
                    sgdisk_size += "G";
                } else {
                  sgdisk_size = "0";
                }

                long long requested_bytes = parse_sz_cmd_to_bytes(sgdisk_size);
                std::string old_sz = p.size_cmd;
                p.size_cmd = "0";

                if (validate_capacity_bounds(disk_path, state,
                                             requested_bytes)) {
                  p.size_cmd = sgdisk_size;
                } else {
                  p.size_cmd = old_sz;
                  DialogBox::show_alert(
                      _tr("Error"),
                      _tr("Cannot resize: Requested size exceeds remaining drive capacity (+ Failsafe boundary)."),
                      banner_func);
                }
              }
            }
          } else if (prop_sel == 1) { // FS
            std::vector<std::string> fs_choices = {"ext4", "btrfs", "vfat",
                                                   "xfs",  "swap",  "None"};
            int fs_sel = DialogBox::ask_selection(
                _tr("Filesystem"), _tr("Select filesystem format:"), fs_choices);
            if (fs_sel != -1) {
              p.fs_type = fs_choices[fs_sel];
              if (p.fs_type == "vfat")
                p.type_code = "ef00";
              else if (p.fs_type == "swap")
                p.type_code = "8200";
              else
                p.type_code = "8300";
            }
          } else if (prop_sel == 2) { // Mount Point
            while (true) {
              std::string mpt = DialogBox::ask_input(_tr("Mount Point"), _tr("Enter Mount Point (e.g. /home):"), p.mount_point);
              if (mpt.empty()) {
                std::vector<std::string> conf_choices = {_tr("No, edit Mount Point"), _tr("Yes, leave blank")};
                int conf_sel = DialogBox::ask_selection(_tr("Mount Point"), "", conf_choices, banner_func, _tr("You entered an empty mount point.\nAre you sure you want to leave it unmounted?"));
                if (conf_sel == 1) { 
                  p.mount_point = mpt;
                  break; 
                }
              } else {
                p.mount_point = mpt;
                break;
              }
            }
          } else if (prop_sel == 3) { // Mount Options
            bool opt_loop = true;
            while (opt_loop) {
              std::string wiki_rec = uli::partitioner::identifiers::
                  FlagPartitions::get_wiki_recommendation(p.fs_type);
              std::string mopts = DialogBox::ask_input(
                  _tr("Mount Options"),
                  _tr("Enter Mount Options (e.g. defaults,noatime).\n") + wiki_rec, p.mount_options);
              if (mopts.empty())
                break;

              auto lint =
                  uli::partitioner::identifiers::FlagPartitions::lint_options(
                      p.fs_type, mopts);
              if (!lint.is_valid) {
                DialogBox::show_alert(_tr("Invalid Mount Option"),
                                      lint.error_message, banner_func);
                continue;
              }

              p.mount_options = mopts;
              opt_loop = false;
            }
          } else if (prop_sel == 4) { // Label
            std::string lbl =
                DialogBox::ask_input(_tr("Volume Label"), _tr("Enter new Volume Label:"), p.label);
            p.label = lbl;
          } else if (prop_sel == 5) { // Delete
            state.partitions.erase(state.partitions.begin() + p_idx);
            break;
          }
        }
      } else if (action_sel == 0) { // Create new
        std::vector<std::string> units = {"MB", "MiB", "GB", "GiB"};
        int unit_sel = DialogBox::ask_selection(
            "Unit of Measurement", "Select measuring metric:", units);
        if (unit_sel == -1)
          continue;
        std::string unit = units[unit_sel];

        std::string size_input = DialogBox::ask_input(
            _tr("Partition Size"), _tr("Enter partition size in ") + unit +
                                  _tr(" (e.g. +512, or 0 for remaining space):"));
        if (size_input.empty())
          continue;

        std::string sgdisk_size = "";
        if (size_input == "0" || size_input == "+0") {
          sgdisk_size = "0";
        } else {
          if (size_input[0] != '+')
            size_input = "+" + size_input;
          if (unit == "MB" || unit == "MiB")
            size_input += "M";
          if (unit == "GB" || unit == "GiB")
            size_input += "G";
          sgdisk_size = size_input;
        }

        long long requested_bytes = parse_sz_cmd_to_bytes(sgdisk_size);
        if (!validate_capacity_bounds(disk_path, state, requested_bytes)) {
          DialogBox::show_alert(_tr("Error"),
                                _tr("Capacity Failsafe: Not enough unallocated space on drive (Padding constraints applied)."),
                                banner_func);
          continue;
        }

        std::vector<std::string> fs_choices = {"ext4", "btrfs", "vfat", "xfs",
                                               "swap"};
        int fs_sel = DialogBox::ask_selection(
            "Filesystem", "Select filesystem format:", fs_choices);
        if (fs_sel == -1)
          continue;
        std::string fs_type = fs_choices[fs_sel];

        std::string type_code = "8300"; // Linux filesystem
        if (fs_type == "vfat")
          type_code = "ef00"; // EFI System
        if (fs_type == "swap")
          type_code = "8200"; // Linux swap

        std::string label = DialogBox::ask_input(
            _tr("Volume Label"), _tr("Set Volume Label (optional):"));

        PartitionConfig cfg;
        cfg.part_num = part_num;
        cfg.fs_type = fs_type;
        cfg.label = label;
        cfg.size_cmd = sgdisk_size;
        cfg.type_code = type_code;
        cfg.is_deferred = true;
        cfg.device_path = disk_path; // Master target disk reference

        if (fs_type == "vfat")
          cfg.mount_point = (os_distro == "Debian") ? "/boot/efi" : "/boot";

        else if (fs_type == "swap")
          cfg.mount_point = "[SWAP]";
        else if (part_num == 1 || (part_num == 2 && type_code != "ef00"))
          cfg.mount_point = "/";
        else {
          while (true) {
            std::string mpt = DialogBox::ask_input(
                _tr("Mount Point"), _tr("Enter Mount Point (e.g. /home):"));
            if (mpt.empty()) {
              std::vector<std::string> conf_choices = {_tr("No, edit Mount Point"), _tr("Yes, leave blank")};
              int conf_sel = DialogBox::ask_selection(_tr("Mount Point"), "", conf_choices, banner_func, _tr("You entered an empty mount point.\nAre you sure you want to leave it unmounted?"));
              if (conf_sel == 1) { 
                cfg.mount_point = mpt;
                break; 
              }
            } else {
              cfg.mount_point = mpt;
              break;
            }
          }
        }

        if (fs_type != "swap" && cfg.mount_point != "[SWAP]") {
          bool opt_loop = true;
          while (opt_loop) {
            std::string wiki_rec = uli::partitioner::identifiers::
                FlagPartitions::get_wiki_recommendation(fs_type);
            std::string mopts = DialogBox::ask_input(
                _tr("Mount Options"), _tr("Enter Mount Options (comma-separated). Leave empty for \"defaults\".\n") +
                                     wiki_rec);
            if (mopts.empty()) {
              cfg.mount_options = "defaults";
              opt_loop = false;
            } else {
              auto lint =
                  uli::partitioner::identifiers::FlagPartitions::lint_options(
                      fs_type, mopts);
              if (!lint.is_valid) {
                DialogBox::show_alert(_tr("Invalid Mount Option"),
                                      lint.error_message, banner_func);
                continue;
              }
              cfg.mount_options = mopts;
              opt_loop = false;
            }
          }
        } else if (fs_type == "swap") {
          cfg.mount_options = "defaults";
        }

        state.partitions.push_back(cfg);
        part_num++;
      }
    }
  }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_MANUAL_PART_HPP
