#pragma once
// graphics_page.hpp - Graphics Hardware detection and driver selection
#include "../../ncurseslib.hpp"
#include "page.hpp"
#include <string>
#include <vector>

struct GPUInfo {
  std::string name;
  std::string vendor;
  std::string driver_rec;
};

class GraphicsPage : public Page {
  std::vector<GPUInfo> gpus_;
  int selected_ = 0;

  // Driver options per vendor
  struct DriverChoice {
    std::string package;
    std::string label;
    bool selected;
  };
  std::vector<std::vector<DriverChoice>> driver_options_;
  int driver_cursor_ = 0;
  int focus_ = 0; // 0=gpu list, 1=driver selection

  void init_drivers() {
    driver_options_.resize(gpus_.size());
    for (size_t i = 0; i < gpus_.size(); i++) {
      auto &gpu = gpus_[i];
      if (gpu.vendor == "NVIDIA") {
        driver_options_[i] = {
            {"nvidia", "NVIDIA Proprietary (recommended)", true},
            {"nvidia-open", "NVIDIA Open Kernel Modules", false},
            {"nouveau", "Nouveau (open source)", false},
            {"(none)", "No driver", false},
        };
      } else if (gpu.vendor == "AMD") {
        driver_options_[i] = {
            {"xf86-video-amdgpu", "AMDGPU (recommended)", true},
            {"(none)", "No driver (mesa only)", false},
        };
      } else if (gpu.vendor == "Intel") {
        driver_options_[i] = {
            {"xf86-video-intel", "Intel DDX (xf86-video-intel)", false},
            {"mesa", "Mesa (recommended)", true},
            {"(none)", "No driver (modesetting)", false},
        };
      } else if (gpu.vendor.find("adreno") != std::string::npos) {
        driver_options_[i] = {
            {"mesa", "Mesa (recommended)", true},
            {"(none)", "No driver", false},
        };
      } else {
        driver_options_[i] = {
            {"(none)", "No driver", true},
        };
      }
    }
  }

public:
  GraphicsPage(const std::vector<GPUInfo> &gpus) : gpus_(gpus) {
    init_drivers();
  }

  std::string title() const override { return "Graphics Hardware"; }

  void render(WINDOW *win) override {
    int h, w;
    getmaxyx(win, h, w);

    wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
    mvwprintw(win, 1, 2, "Graphics Hardware Detection");
    wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

    NcursesLib::draw_hline(win, 2, 1, w - 2);

    // GPU list
    wattron(win, COLOR_PAIR(CP_SECTION_TITLE));
    mvwprintw(win, 3, 2, "Detected GPUs:");
    wattroff(win, COLOR_PAIR(CP_SECTION_TITLE));

    for (int i = 0; i < (int)gpus_.size() && i < h - 10; i++) {
      int y = 4 + i;
      if (focus_ == 0 && i == selected_) {
        wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
        mvwhline(win, y, 2, ' ', w - 4);
        mvwprintw(win, y, 3, "[%s] %s", gpus_[i].vendor.c_str(),
                  gpus_[i].name.c_str());
        wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
      } else {
        wattron(win, COLOR_PAIR(i == selected_ ? CP_CHECKBOX_ON : CP_NORMAL));
        mvwprintw(win, y, 3, "%s[%s] %s", i == selected_ ? "> " : "  ",
                  gpus_[i].vendor.c_str(), gpus_[i].name.c_str());
        wattroff(win, COLOR_PAIR(i == selected_ ? CP_CHECKBOX_ON : CP_NORMAL));
      }
    }

    NcursesLib::draw_hline(win, 4 + (int)gpus_.size() + 1, 1, w - 2);

    // Driver options for selected GPU
    int drv_y = 4 + (int)gpus_.size() + 2;
    wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
    mvwprintw(win, drv_y, 2, "Driver Options for: %s",
              gpus_[selected_].name.c_str());
    wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

    if (selected_ < (int)driver_options_.size()) {
      auto &opts = driver_options_[selected_];
      for (int i = 0; i < (int)opts.size() && drv_y + 1 + i < h - 2; i++) {
        int y = drv_y + 1 + i;
        auto &d = opts[i];

        if (focus_ == 1 && i == driver_cursor_) {
          wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
          mvwhline(win, y, 2, ' ', w - 4);
        }

        int chk_cp = d.selected ? CP_CHECKBOX_ON : CP_CHECKBOX_OFF;
        if (focus_ != 1 || i != driver_cursor_)
          wattron(win, COLOR_PAIR(chk_cp));
        mvwprintw(win, y, 3, "%s", d.selected ? "(*)" : "( )");
        if (focus_ != 1 || i != driver_cursor_)
          wattroff(win, COLOR_PAIR(chk_cp));

        if (focus_ == 1 && i == driver_cursor_) {
          mvwprintw(win, y, 7, "%-20s  %s", d.package.c_str(), d.label.c_str());
          wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
        } else {
          wattron(win, COLOR_PAIR(CP_NORMAL));
          mvwprintw(win, y, 7, "%-20s  %s", d.package.c_str(), d.label.c_str());
          wattroff(win, COLOR_PAIR(CP_NORMAL));
        }
      }
    }
  }

  bool handle_input(WINDOW *win, int ch) override {
    (void)win;
    if (ch == '\t') {
      focus_ = 1 - focus_;
      if (focus_ == 1)
        driver_cursor_ = 0;
      return true;
    }

    if (focus_ == 0) {
      if (ch == KEY_UP && selected_ > 0) {
        selected_--;
        driver_cursor_ = 0;
        return true;
      }
      if (ch == KEY_DOWN && selected_ < (int)gpus_.size() - 1) {
        selected_++;
        driver_cursor_ = 0;
        return true;
      }
    } else {
      auto &opts = driver_options_[selected_];
      if (ch == KEY_UP && driver_cursor_ > 0) {
        driver_cursor_--;
        return true;
      }
      if (ch == KEY_DOWN && driver_cursor_ < (int)opts.size() - 1) {
        driver_cursor_++;
        return true;
      }
      if (ch == '\n' || ch == KEY_ENTER || ch == ' ') {
        // Radio-button: only one selected per GPU
        for (auto &d : opts)
          d.selected = false;
        opts[driver_cursor_].selected = true;
        return true;
      }
    }
    return false;
  }
};
