#pragma once
#include "../../configurations/datastore.hpp"
#include "../../ncurseslib.hpp"
#include "page.hpp"
#include "../../popups.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>

enum class DiskSection { SelectDisk, ManageDisk, EditPartition };

class DiskPage : public Page {
  DiskSection section_ = DiskSection::SelectDisk;
  int selected_disk_ = 0;
  int selected_part_ = 0;
  int selected_part_field_ = 0;
  int part_scroll_ = 0;

  static int fs_color(const std::string &fs) {
    if (fs == "ext4") return CP_DISK_EXT4;
    if (fs == "fat32" || fs == "vfat") return CP_DISK_FAT32;
    if (fs == "btrfs") return CP_DISK_BTRFS;
    if (fs == "swap") return CP_DISK_SWAP;
    if (fs == "xfs") return CP_DISK_XFS;
    if (fs == "ntfs") return CP_DISK_NTFS;
    return CP_DISK_UNALLOC;
  }

  static std::string fmt_size(uint64_t mb) {
    char buf[32];
    if (mb >= 1024) snprintf(buf, sizeof(buf), "%.1fGB", mb / 1024.0);
    else snprintf(buf, sizeof(buf), "%luMB", (unsigned long)mb);
    return buf;
  }

public:
  DiskPage() {}

  std::string title() const override {
    if (section_ == DiskSection::SelectDisk) return "Storage Device > Select Disk";
    if (section_ == DiskSection::ManageDisk) return "Storage Device > Manage Partitions";
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
    case DiskSection::SelectDisk: render_select_disk(win); break;
    case DiskSection::ManageDisk: render_manage_disk(win); break;
    case DiskSection::EditPartition: render_edit_partition(win); break;
    }
  }

  bool handle_input(WINDOW *win, int ch) override {
    if (NcursesLib::is_back_key(ch)) {
      if (section_ == DiskSection::EditPartition) { section_ = DiskSection::ManageDisk; return true; }
      if (section_ == DiskSection::ManageDisk) { section_ = DiskSection::SelectDisk; return true; }
      return false;
    }
    switch (section_) {
    case DiskSection::SelectDisk: return handle_select_disk(ch);
    case DiskSection::ManageDisk: return handle_manage_disk(ch);
    case DiskSection::EditPartition: return handle_edit_partition(ch);
    }
    return false;
  }

private:
  void render_select_disk(WINDOW *win) {
    int h, w; getmaxyx(win, h, w);
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
        mvwprintw(win, y, 4, "> %-10s  %10s  %s", d.device.c_str(), fmt_size(d.size_mb).c_str(), d.model.c_str());
        wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
      } else mvwprintw(win, y, 4, "  %-10s  %10s  %s", d.device.c_str(), fmt_size(d.size_mb).c_str(), d.model.c_str());
    }
  }

  void render_manage_disk(WINDOW *win) {
    int h, w; getmaxyx(win, h, w);
    const auto &disk = DataStore::instance().disks[selected_disk_];
    wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
    mvwprintw(win, 1, 2, "Disk: %s (%s)", disk.device.c_str(), fmt_size(disk.size_mb).c_str());
    wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
    int table_y = 6;
    wattron(win, COLOR_PAIR(CP_TABLE_HEADER) | A_BOLD | A_UNDERLINE);
    mvwprintw(win, table_y, 2, "   %-12s %-10s %10s  %-10s  %-12s", "Device", "Mount", "Size", "FS", "Flags");
    wattroff(win, COLOR_PAIR(CP_TABLE_HEADER) | A_BOLD | A_UNDERLINE);
    int max_items = disk.partitions.size() + 2;
    int list_h = h - table_y - 2;
    if (selected_part_ < part_scroll_) part_scroll_ = selected_part_;
    if (selected_part_ >= part_scroll_ + list_h) part_scroll_ = selected_part_ - list_h + 1;
    for (int i = 0; i < list_h && (part_scroll_ + i) < max_items; i++) {
      int idx = part_scroll_ + i;
      int ry = table_y + 1 + i;
      bool is_sel = (idx == selected_part_);
      if (is_sel) { wattron(win, COLOR_PAIR(CP_HIGHLIGHT)); mvwhline(win, ry, 2, ' ', w - 4); }
      if (idx < (int)disk.partitions.size()) {
        auto &p = disk.partitions[idx];
        mvwprintw(win, ry, 6, "%-12s %-10s %10s  %-10s  %-12s", p.device.c_str(), p.mount_point.c_str(), fmt_size(p.size_mb).c_str(), p.filesystem.c_str(), p.flags.c_str());
      } else if (idx == (int)disk.partitions.size()) mvwprintw(win, ry, 6, "[+ Create New Partition]");
      else mvwprintw(win, ry, 6, "[ BACK ]");
      if (is_sel) wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
    }
  }

  void render_edit_partition(WINDOW *win) {
    int h, w; getmaxyx(win, h, w);
    auto &p = DataStore::instance().disks[selected_disk_].partitions[selected_part_];
    wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
    mvwprintw(win, 1, 2, "Edit Partition: %s", p.device.c_str());
    wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
    mvwprintw(win, 4, 4, "Role:        %s", p.mount_point == "/boot/efi" ? "EFI System" : p.mount_point == "/" ? "Root" : "Custom");
    mvwprintw(win, 6, 4, "Mount Point: %s", p.mount_point.c_str());
    mvwprintw(win, 8, 4, "Filesystem:  %s", p.filesystem.c_str());
    mvwprintw(win, 10, 4, "Flags:       %s", p.flags.c_str());
    mvwprintw(win, 12, 4, "Size (MB):   %lu", p.size_mb);
    const char* actions[] = { "[ SAVE ]", "[ CANCEL ]" };
    for (int i = 0; i < 2; i++) {
        int y = 14 + (i * 2);
        if (selected_part_field_ == 5 + i) {
            wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
            mvwhline(win, y, 4, ' ', w - 8);
            mvwprintw(win, y, 6, "%s", actions[i]);
            wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
        } else mvwprintw(win, y, 6, "%s", actions[i]);
    }
    if (selected_part_field_ < 5) {
        wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
        int y = 4 + (selected_part_field_ * 2);
        mvwhline(win, y, 4, ' ', w - 8);
        if (selected_part_field_ == 0) mvwprintw(win, y, 4, "Role:        [ Select ]");
        else if (selected_part_field_ == 1) mvwprintw(win, y, 4, "Mount Point: %s", p.mount_point.c_str());
        else if (selected_part_field_ == 2) mvwprintw(win, y, 4, "Filesystem:  %s", p.filesystem.c_str());
        else if (selected_part_field_ == 3) mvwprintw(win, y, 4, "Flags:       %s", p.flags.c_str());
        else if (selected_part_field_ == 4) mvwprintw(win, y, 4, "Size (MB):   %lu", p.size_mb);
        wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
    }
  }

  bool handle_select_disk(int ch) {
    if (ch == KEY_UP && selected_disk_ > 0) { selected_disk_--; return true; }
    if (ch == KEY_DOWN && selected_disk_ < (int)DataStore::instance().disks.size() - 1) { selected_disk_++; return true; }
    if (ch == '\n' || ch == KEY_ENTER) { section_ = DiskSection::ManageDisk; selected_part_ = 0; return true; }
    return false;
  }

  bool handle_manage_disk(int ch) {
    int max_items = DataStore::instance().disks[selected_disk_].partitions.size() + 2;
    if (ch == KEY_UP && selected_part_ > 0) { selected_part_--; return true; }
    if (ch == KEY_DOWN && selected_part_ < max_items - 1) { selected_part_++; return true; }
    if (ch == '\n' || ch == KEY_ENTER) {
      if (selected_part_ < (int)DataStore::instance().disks[selected_disk_].partitions.size()) {
        section_ = DiskSection::EditPartition; selected_part_field_ = 0;
      } else if (selected_part_ == (int)DataStore::instance().disks[selected_disk_].partitions.size()) {
         /* Add logic for new partition */
      } else section_ = DiskSection::SelectDisk;
      return true;
    }
    return false;
  }

  bool handle_edit_partition(int ch) {
    if (ch == KEY_UP && selected_part_field_ > 0) { selected_part_field_--; return true; }
    if (ch == KEY_DOWN && selected_part_field_ < 6) { selected_part_field_++; return true; }
    if (ch == '\n' || ch == KEY_ENTER) {
        auto &p = DataStore::instance().disks[selected_disk_].partitions[selected_part_];
        if (selected_part_field_ == 1) { std::string res = InputPopup::show("Mount Point", "Enter path:", p.mount_point); if(!res.empty()) p.mount_point = res; }
        else if (selected_part_field_ == 2) { std::string res = InputPopup::show("Filesystem", "Enter type:", p.filesystem); if(!res.empty()) p.filesystem = res; }
        else if (selected_part_field_ == 3) { std::string res = InputPopup::show("Flags", "Enter flags:", p.flags); if(!res.empty()) p.flags = res; }
        else if (selected_part_field_ == 4) { std::string res = InputPopup::show("Resize", "Enter MB:", std::to_string(p.size_mb)); if(!res.empty()) p.size_mb = std::stoull(res); }
        else if (selected_part_field_ == 5) { section_ = DiskSection::ManageDisk; return true; }
        else if (selected_part_field_ == 6) { section_ = DiskSection::ManageDisk; return true; }
    }
    return false;
  }
};
