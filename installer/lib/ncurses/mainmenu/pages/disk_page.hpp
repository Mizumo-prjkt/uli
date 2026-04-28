#pragma once
#include "../../configurations/datastore.hpp"
#include "../../ncurseslib.hpp"
#include "../../popups.hpp"
#include "page.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

enum class DiskSection { SelectDisk, ManageDisk };

class DiskPage : public Page {
  DiskSection section_ = DiskSection::SelectDisk;
  int selected_disk_ = 0;
  int selected_part_ = 0;
  int selected_part_field_ = 0;
  int part_scroll_ = 0;

  static int fs_color(const std::string &fs) {
    if (fs == "ext4")
      return CP_DISK_EXT4;
    if (fs == "fat32" || fs == "vfat")
      return CP_DISK_FAT32;
    if (fs == "btrfs")
      return CP_DISK_BTRFS;
    if (fs == "swap")
      return CP_DISK_SWAP;
    if (fs == "xfs")
      return CP_DISK_XFS;
    if (fs == "ntfs")
      return CP_DISK_NTFS;
    return CP_DISK_UNALLOC;
  }

  static std::string fmt_size(uint64_t mb) {
    char buf[32];
    if (mb >= 1024)
      snprintf(buf, sizeof(buf), "%.1fGB", mb / 1024.0);
    else
      snprintf(buf, sizeof(buf), "%luMB", (unsigned long)mb);
    return buf;
  }

public:
  DiskPage() {}

  std::string title() const override {
    if (section_ == DiskSection::SelectDisk)
      return "Storage Device > Select Disk";
    if (section_ == DiskSection::ManageDisk)
      return "Storage Device > Manage Partitions";
    return "Storage Device > Edit Partition";
  }

  bool has_pending_changes() const override {
    return section_ != DiskSection::SelectDisk;
  }

  void discard_pending_changes() override {
    section_ = DiskSection::SelectDisk;
  }

  void render(WINDOW *win) override {
    switch (section_) {
    case DiskSection::SelectDisk:
      render_select_disk(win);
      break;
    case DiskSection::ManageDisk:
      render_manage_disk(win);
      break;
    }
  }

  bool handle_input(WINDOW *win, int ch) override {
    if (NcursesLib::is_back_key(ch)) {
      if (section_ == DiskSection::ManageDisk) {
        section_ = DiskSection::SelectDisk;
        return true;
      }
      return false;
    }
    switch (section_) {
    case DiskSection::SelectDisk:
      return handle_select_disk(ch);
    case DiskSection::ManageDisk:
      return handle_manage_disk(ch);
    }
    return false;
  }

private:
  void draw_disk_bar(WINDOW *win, int y, int x, int w) {
    const auto &disk = DataStore::instance().disks[selected_disk_];
    uint64_t total = disk.size_mb;
    if (total == 0)
      return;
    int bar_max_w = w - 6;
    int curr_x = x + 1;

    for (const auto &p : disk.partitions) {
      int bar_w = (p.size_mb * bar_max_w) / total;
      if (bar_w < 1 && p.size_mb > 0)
        bar_w = 1;

      int color = fs_color(p.filesystem);
      wattron(win, COLOR_PAIR(color));
      for (int i = 0; i < bar_w && (curr_x + i) < (x + bar_max_w + 1); i++) {
        mvwaddch(win, y, curr_x + i, ' ');
        mvwaddch(win, y + 1, curr_x + i, ' ');
      }

      if (bar_w > 6) {
        std::string label = p.device;
        if (label.size() > (size_t)bar_w - 2)
          label = label.substr(0, bar_w - 2);
        mvwprintw(win, y, curr_x + (bar_w - label.size()) / 2, "%s",
                  label.c_str());
      }
      wattroff(win, COLOR_PAIR(color));
      curr_x += bar_w;
    }

    // Fill remaining with unallocated
    if (curr_x < x + bar_max_w + 1) {
      wattron(win, COLOR_PAIR(CP_DISK_UNALLOC));
      for (int i = 0; curr_x + i < (x + bar_max_w + 1); i++) {
        mvwaddch(win, y, curr_x + i, ' ');
        mvwaddch(win, y + 1, curr_x + i, ' ');
      }
      wattroff(win, COLOR_PAIR(CP_DISK_UNALLOC));
    }

    // Draw border around bar
    wattron(win, COLOR_PAIR(CP_CONTENT_BORDER));
    mvwaddch(win, y, x, ACS_VLINE);
    mvwaddch(win, y + 1, x, ACS_VLINE);
    mvwaddch(win, y, x + bar_max_w + 1, ACS_VLINE);
    mvwaddch(win, y + 1, x + bar_max_w + 1, ACS_VLINE);
    mvwhline(win, y - 1, x, ACS_HLINE, bar_max_w + 2);
    mvwhline(win, y + 2, x, ACS_HLINE, bar_max_w + 2);
    mvwaddch(win, y - 1, x, ACS_ULCORNER);
    mvwaddch(win, y - 1, x + bar_max_w + 1, ACS_URCORNER);
    mvwaddch(win, y + 2, x, ACS_LLCORNER);
    mvwaddch(win, y + 2, x + bar_max_w + 1, ACS_LRCORNER);
    wattroff(win, COLOR_PAIR(CP_CONTENT_BORDER));
  }

  void render_select_disk(WINDOW *win) {
    int h, w;
    getmaxyx(win, h, w);
    wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
    mvwprintw(win, 1, 2, "Select Storage Device");
    wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
    NcursesLib::draw_hline(win, 2, 1, w - 2);
    for (int i = 0; i < (int)DataStore::instance().disks.size(); i++) {
      int y = 4 + i * 2;
      const auto &d = DataStore::instance().disks[i];
      if (i == selected_disk_) {
        wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
        mvwhline(win, y, 2, ' ', w - 4);
        mvwprintw(win, y, 4, "> %-10s  %10s  %s", d.device.c_str(),
                  fmt_size(d.size_mb).c_str(), d.model.c_str());
        wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
      } else
        mvwprintw(win, y, 4, "  %-10s  %10s  %s", d.device.c_str(),
                  fmt_size(d.size_mb).c_str(), d.model.c_str());
    }
  }

  void render_manage_disk(WINDOW *win) {
    int h, w;
    getmaxyx(win, h, w);
    const auto &disk = DataStore::instance().disks[selected_disk_];
    wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
    mvwprintw(win, 1, 2, "Disk: %s (%s)", disk.device.c_str(),
              fmt_size(disk.size_mb).c_str());
    wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

    draw_disk_bar(win, 3, 2, w);

    // Legend
    int lx = 2;
    mvwprintw(win, 6, lx, "Legend: ");
    lx += 8;
    auto draw_legend = [&](const std::string &name, int cp) {
      wattron(win, COLOR_PAIR(cp));
      mvwprintw(win, 6, lx, "  ");
      lx += 3;
      wattroff(win, COLOR_PAIR(cp));
      mvwprintw(win, 6, lx, "%s  ", name.c_str());
      lx += name.size() + 2;
    };
    draw_legend("ext4", CP_DISK_EXT4);
    draw_legend("fat32", CP_DISK_FAT32);
    draw_legend("btrfs", CP_DISK_BTRFS);
    draw_legend("swap", CP_DISK_SWAP);
    draw_legend("xfs", CP_DISK_XFS);
    draw_legend("free", CP_DISK_UNALLOC);

    int table_y = 8;
    wattron(win, COLOR_PAIR(CP_TABLE_HEADER) | A_BOLD | A_UNDERLINE);
    mvwprintw(win, table_y, 2, "   %-10s %-25s %8s  %-8s  %-12s", "Device",
              "Mount", "Size", "FS", "Flags");
    wattroff(win, COLOR_PAIR(CP_TABLE_HEADER) | A_BOLD | A_UNDERLINE);

    int max_items = disk.partitions.size() + 2;
    int list_h = h - table_y - 4; // Room for table and info bar
    if (selected_part_ < part_scroll_)
      part_scroll_ = selected_part_;
    if (selected_part_ >= part_scroll_ + list_h)
      part_scroll_ = selected_part_ - list_h + 1;

    auto truncate = [](const std::string& s, size_t len) {
        if (s.length() <= len) return s;
        return s.substr(0, len-3) + "...";
    };

    for (int i = 0; i < list_h && (part_scroll_ + i) < max_items; i++) {
      int idx = part_scroll_ + i;
      int ry = table_y + 1 + i;
      bool is_sel = (idx == selected_part_);
      if (is_sel) {
        wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
        mvwhline(win, ry, 2, ' ', w - 4);
      }
      if (idx < (int)disk.partitions.size()) {
        auto &p = disk.partitions[idx];
        mvwprintw(win, ry, 6, "%-10s %-25s %8s  %-8s  %-12s",
                  p.device.c_str(), truncate(p.mount_point, 25).c_str(),
                  fmt_size(p.size_mb).c_str(), p.filesystem.c_str(),
                  p.flags.c_str());
      } else if (idx == (int)disk.partitions.size())
        mvwprintw(win, ry, 6, "[+ Create New Partition]");
      else
        mvwprintw(win, ry, 6, "[ BACK ]");
      if (is_sel)
        wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
    }

    // Disk Summary Bar
    wattron(win, COLOR_PAIR(CP_STATUS_BAR));
    mvwhline(win, h - 1, 0, ' ', w);
    
    uint64_t used_mb = 0;
    for (const auto& p : disk.partitions) used_mb += p.size_mb;
    uint64_t free_mb = (disk.size_mb > used_mb) ? (disk.size_mb - used_mb) : 0;
    
    mvwprintw(win, h - 1, 2, "Disk: %s (%s) | Total: %.1f GB | Partitions: %zu | Used: %lu MB | Free: %lu MB",
              disk.device.c_str(), disk.model.c_str(), 
              (double)disk.size_mb / 1024.0,
              disk.partitions.size(),
              used_mb, free_mb);
    wattroff(win, COLOR_PAIR(CP_STATUS_BAR));
  }

  void render_edit_partition(WINDOW *win) {
    int h, w;
    getmaxyx(win, h, w);
    auto &p =
        DataStore::instance().disks[selected_disk_].partitions[selected_part_];
    wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
    mvwprintw(win, 1, 2, "Edit Partition: %s", p.device.c_str());
    wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
    mvwprintw(win, 4, 4, "Role:        %s",
              p.mount_point == "/boot/efi" ? "EFI System"
              : p.mount_point == "/"       ? "Root"
                                           : "Custom");
    mvwprintw(win, 6, 4, "Mount Point: %s", p.mount_point.c_str());
    mvwprintw(win, 8, 4, "Filesystem:  %s", p.filesystem.c_str());
    mvwprintw(win, 10, 4, "Flags:       %s", p.flags.c_str());
    mvwprintw(win, 12, 4, "Size (MB):   %lu", p.size_mb);
    const char *actions[] = {"[ SAVE ]", "[ CANCEL ]"};
    for (int i = 0; i < 2; i++) {
      int y = 14 + (i * 2);
      if (selected_part_field_ == 5 + i) {
        wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
        mvwhline(win, y, 4, ' ', w - 8);
        mvwprintw(win, y, 6, "%s", actions[i]);
        wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
      } else
        mvwprintw(win, y, 6, "%s", actions[i]);
    }
    if (selected_part_field_ < 5) {
      wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
      int y = 4 + (selected_part_field_ * 2);
      mvwhline(win, y, 4, ' ', w - 8);
      if (selected_part_field_ == 0)
        mvwprintw(win, y, 4, "Role:        [ Select ]");
      else if (selected_part_field_ == 1)
        mvwprintw(win, y, 4, "Mount Point: %s", p.mount_point.c_str());
      else if (selected_part_field_ == 2)
        mvwprintw(win, y, 4, "Filesystem:  %s", p.filesystem.c_str());
      else if (selected_part_field_ == 3)
        mvwprintw(win, y, 4, "Flags:       %s", p.flags.c_str());
      else if (selected_part_field_ == 4)
        mvwprintw(win, y, 4, "Size (MB):   %lu", p.size_mb);
      wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
    }
  }

  bool handle_select_disk(int ch) {
    if (ch == KEY_UP && selected_disk_ > 0) {
      selected_disk_--;
      return true;
    }
    if (ch == KEY_DOWN &&
        selected_disk_ < (int)DataStore::instance().disks.size() - 1) {
      selected_disk_++;
      return true;
    }
    if (ch == '\n' || ch == KEY_ENTER) {
      section_ = DiskSection::ManageDisk;
      selected_part_ = 0;
      return true;
    }
    return false;
  }

  bool handle_manage_disk(int ch) {
    auto &ds = DataStore::instance();
    int max_items = ds.disks[selected_disk_].partitions.size() + 2;

    if (ch == KEY_UP && selected_part_ > 0) {
      selected_part_--;
      return true;
    }
    if (ch == KEY_DOWN && selected_part_ < max_items - 1) {
      selected_part_++;
      return true;
    }

    if (ch == KEY_LEFT && selected_disk_ > 0) {
      selected_disk_--;
      selected_part_ = 0;
      return true;
    }
    if (ch == KEY_RIGHT && selected_disk_ < (int)ds.disks.size() - 1) {
      selected_disk_++;
      selected_part_ = 0;
      return true;
    }

    if (ch == 'd' || ch == 'D' || ch == KEY_DC) {
      if (selected_part_ < (int)ds.disks[selected_disk_].partitions.size()) {
        if (YesNoPopup::show(
                "Delete Partition",
                "Are you sure you want to delete " +
                    ds.disks[selected_disk_].partitions[selected_part_].device +
                    "?")) {
          ds.disks[selected_disk_].partitions.erase(
              ds.disks[selected_disk_].partitions.begin() + selected_part_);
          if (selected_part_ > 0)
            selected_part_--;
          return true;
        }
      }
    }

    if (ch == '\n' || ch == KEY_ENTER) {
      if (selected_part_ < (int)ds.disks[selected_disk_].partitions.size()) {
        auto &p = ds.disks[selected_disk_].partitions[selected_part_];

        std::vector<FormField> fields = {
            {"Mount Point", "e.g. /, /home, /boot/efi", p.mount_point, 128,
             FieldType::Text},
            {"Filesystem",
             "Select filesystem type",
             p.filesystem,
             10,
             FieldType::Select,
             {"ext4", "btrfs", "fat32", "swap", "xfs"}},
            {"Subvolume", "Btrfs subvolume names (comma separated)",
             p.btrfs_subvol, 128, FieldType::Text},
            {"Compression",
             "Btrfs compression type",
             p.btrfs_compress,
             10,
             FieldType::Select,
             {"zstd", "lzo", "none"}},
            {"Flags", "e.g. boot, esp", p.flags, 30, FieldType::Text},
            {"Size (MB)", "Size in Megabytes", std::to_string(p.size_mb), 10,
             FieldType::Text}};

        auto update_fn = [](std::vector<FormField> &f) {
          bool is_btrfs = (f[1].value == "btrfs");
          f[2].hidden = !is_btrfs;
          f[3].hidden = !is_btrfs;
        };

        if (FormPopup::show("Edit Partition: " + p.device, fields, update_fn)) {
          p.mount_point = fields[0].value;
          p.filesystem = fields[1].value;
          if (p.filesystem == "btrfs") {
            p.btrfs_subvol = fields[2].value;
            p.btrfs_compress = fields[3].value;
          }
          p.flags = fields[4].value;
          try {
            p.size_mb = std::stoull(fields[5].value);
          } catch (...) {
          }
        }
      } else if (selected_part_ ==
                 (int)ds.disks[selected_disk_].partitions.size()) {
        // Create New Partition
        std::string size_str =
            InputPopup::show("New Partition", "Enter size in MB:", "1024");
        if (!size_str.empty()) {
          DiskPartition np;
          np.device =
              ds.disks[selected_disk_].device +
              std::to_string(ds.disks[selected_disk_].partitions.size() + 1);
          np.mount_point = "/mnt/new";
          np.filesystem = "ext4";
          np.size_mb = std::stoull(size_str);
          ds.disks[selected_disk_].partitions.push_back(np);
        }
      } else
        section_ = DiskSection::SelectDisk;
      return true;
    }
    return false;
  }
};
