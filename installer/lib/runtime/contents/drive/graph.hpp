#ifndef ULI_RUNTIME_DRIVE_GRAPH_HPP
#define ULI_RUNTIME_DRIVE_GRAPH_HPP

#include "../../../partitioner/diskcheck.hpp"
#include "../../term_capable_check.hpp"
#include "../translations/lang_export.hpp"
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

namespace uli {
namespace runtime {
namespace contents {
namespace drive {

class DiskGraph {
public:
  static std::string get_disk_layout_string(const std::string &disk_path) {
    long long total_bytes =
        uli::partitioner::DiskCheck::get_disk_size_bytes(disk_path);
    if (total_bytes == 0) {
      return "Unable to read disk size for " + disk_path + "\n";
    }

    std::vector<uli::partitioner::PartitionInfo> parts =
        uli::partitioner::DiskCheck::get_disk_partitions(disk_path);

    return get_disk_layout_string_from_parts(disk_path, total_bytes, parts);
  }

  static std::string get_disk_layout_string_from_parts(const std::string &disk_path, long long total_bytes, const std::vector<uli::partitioner::PartitionInfo> &parts) {
    std::stringstream ss;

    int cols, rows;
    TermCapableCheck::get_terminal_size(cols, rows);
    bool use_color = TermCapableCheck::supports_color();

    // Calculate usable width for the bar chart based on term dimensions
    int bar_width = cols - 10;
    if (bar_width < 20)
      bar_width = 20;

    ss << "\n" << _tr("Disk Target: ") << disk_path << " (" << format_bytes(total_bytes)
       << ")\n";

        if (parts.empty()) {
            std::string empty_label = _tr("Unpartitioned / Empty");
            ss << "[";
            if (use_color) ss << "\033[1;30m"; // Dark grey empty layout
            
            int left_dash = (bar_width - empty_label.length()) / 2;
            int right_dash = bar_width - empty_label.length() - left_dash;
            if (left_dash < 0) left_dash = 0;
            if (right_dash < 0) right_dash = 0;
            
            for (int i=0; i<left_dash; ++i) ss << "-";
            ss << empty_label;
            for (int i=0; i<right_dash; ++i) ss << "-";
            
            if (use_color) ss << "\033[0m";
            ss << "]\n\n";
            return ss.str();
        }

        long long used_bytes = 0;
        for (const auto& p : parts) {
            used_bytes += p.size_bytes;
        }

        struct Label {
            int arrow_pos;
            int text_start;
            std::string text;
            int color;
        };

        std::vector<std::vector<Label>> label_rows;
        int colors[] = {31, 32, 33, 34, 35, 36}; // Red, Green, Yellow, Blue, Magenta, Cyan
        int current_pos = 0;

        for (size_t i = 0; i < parts.size(); ++i) {
            int part_chars = (int)((double)parts[i].size_bytes / total_bytes * bar_width);
            if (part_chars < 1) part_chars = 1;
            if (current_pos + part_chars > bar_width) part_chars = bar_width - current_pos;

            int arrow_pos = current_pos + part_chars / 2;
            std::string text = "/dev/" + parts[i].name;
            int text_start = arrow_pos - text.length() / 2;
            if (text_start < 0) text_start = 0;
            if (text_start + text.length() > bar_width) text_start = bar_width - text.length();

            int placed_row = -1;
            for (size_t r = 0; r < label_rows.size(); ++r) {
                bool overlap = false;
                for (const auto& l : label_rows[r]) {
                    if (!(text_start + text.length() + 1 <= l.text_start || text_start >= l.text_start + l.text.length() + 1)) {
                        overlap = true;
                        break;
                    }
                }
                if (!overlap) {
                    placed_row = r;
                    break;
                }
            }

            if (placed_row == -1) {
                label_rows.push_back({});
                placed_row = label_rows.size() - 1;
            }

            label_rows[placed_row].push_back({arrow_pos, text_start, text, colors[i % 6]});
            current_pos += part_chars;
        }

        // Print labels from highest row to lowest
        for (int r = label_rows.size() - 1; r >= 0; --r) {
            ss << " "; // padding for '['
            auto sorted_row = label_rows[r];
            std::sort(sorted_row.begin(), sorted_row.end(), [](const Label& a, const Label& b) { return a.text_start < b.text_start; });
            
            int cursor = 0;
            for (const auto& l : sorted_row) {
                if (l.text_start > cursor) {
                    ss << std::string(l.text_start - cursor, ' ');
                    cursor = l.text_start;
                }
                if (use_color) ss << "\033[1;" << l.color << "m";
                ss << l.text;
                if (use_color) ss << "\033[0m";
                cursor += l.text.length();
            }
            ss << "\n";
        }

        // Print arrows
        ss << " "; // padding for '['
        std::vector<Label> all_arrows;
        for (const auto& r : label_rows) {
            for (const auto& l : r) all_arrows.push_back(l);
        }
        std::sort(all_arrows.begin(), all_arrows.end(), [](const Label& a, const Label& b) { return a.arrow_pos < b.arrow_pos; });

        int cursor = 0;
        for (const auto& l : all_arrows) {
            if (l.arrow_pos > cursor) {
                ss << std::string(l.arrow_pos - cursor, ' ');
                cursor = l.arrow_pos;
            }
            if (use_color) ss << "\033[1;" << l.color << "m";
            ss << "v";
            if (use_color) ss << "\033[0m";
            cursor++;
        }
        ss << "\n";

        // Draw active capacity usage layout blocks
        ss << "[";
        int drawn = 0;
        for (size_t i = 0; i < parts.size(); ++i) {
            int part_chars = (int)((double)parts[i].size_bytes / total_bytes * bar_width);
            if (part_chars < 1) part_chars = 1;
            if (drawn + part_chars > bar_width) part_chars = bar_width - drawn;
            
            if (use_color) ss << "\033[1;" << colors[i % 6] << "m"; // Match partition color
            for (int c = 0; c < part_chars; ++c) ss << "#";
            if (use_color) ss << "\033[0m";
            drawn += part_chars;
        }

        long long free_bytes = total_bytes - used_bytes;
        if (free_bytes > 0 && drawn < bar_width) {
            int free_chars = bar_width - drawn;
      if (use_color)
        ss << "\033[1;30m";
      for (int i = 0; i < free_chars; ++i)
        ss << "-";
      if (use_color)
        ss << "\033[0m";
    }
    ss << "]\n\n";

    // Print textual boundary summaries underneath the graph
    ss << " --------------------------------------------------------------------------------\n";
    ss << "  " << std::left << std::setw(20) << _tr("Partition") << " | "
       << std::setw(11) << _tr("Size") << " | "
       << std::setw(10) << _tr("FS Type") << " | "
       << std::setw(11) << _tr("Label") << " | "
       << _tr("Mount") << "\n";
    ss << " --------------------------------------------------------------------------------\n";
    for (const auto &p : parts) {
      char buf[256];
      snprintf(buf, sizeof(buf), "  %-20s | %-11s | %-10s | %-11s | %-11s\n", ("/dev/" + p.name).c_str(), format_bytes(p.size_bytes).c_str(), p.fs_type.empty() ? "-" : p.fs_type.c_str(), p.label.empty() ? "-" : p.label.c_str(), p.mount_point.empty() ? "-" : p.mount_point.c_str());
      ss << buf;
    }
    char fbuf[256];
    snprintf(fbuf, sizeof(fbuf), "  %-20s | %-11s | %-10s | %-11s | %-11s\n", _tr("Free Space").c_str(), format_bytes(free_bytes).c_str(), "-", "-", "-");
    ss << fbuf;
    ss << " --------------------------------------------------------------------------------\n\n";

    return ss.str();
  }

public:
  // Format large byte boundaries into readable strings (MB, GB, TB)
  static std::string format_bytes(long long bytes) {
    const char *suffixes[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double dbytes = bytes;
    while (dbytes >= 1024 && i < 4) {
      dbytes /= 1024;
      i++;
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f %s", dbytes, suffixes[i]);
    return std::string(buf);
  }
};

} // namespace drive
} // namespace contents
} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_DRIVE_GRAPH_HPP
