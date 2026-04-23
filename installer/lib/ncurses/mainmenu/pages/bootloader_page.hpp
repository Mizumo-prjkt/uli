#pragma once
// bootloader_page.hpp - Bootloader Selection page
#include "page.hpp"
#include "../../ncurseslib.hpp"
#include <vector>
#include <string>

struct BootloaderOption {
    std::string name;
    std::string description;
};

class BootloaderPage : public Page {
    std::vector<BootloaderOption> options_;
    int selected_ = 0;

public:
    BootloaderPage() {
        options_ = {
            {"GRUB",         "The GNU Grand Unified Bootloader. Highly compatible and feature-rich."},
            {"systemd-boot", "A simple UEFI boot manager. Fast and minimalist. (UEFI only)"},
            {"None",         "Do not install a bootloader. (Expert only)"}
        };
    }

    std::string title() const override { return "Bootloader Selection"; }

    void render(WINDOW* win) override {
        int h, w;
        getmaxyx(win, h, w);

        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Select Bootloader");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        for (int i = 0; i < (int)options_.size(); i++) {
            int y = 3 + i * 2;
            bool is_sel = (i == selected_);

            if (is_sel) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y, 2, ' ', w - 4);
                mvwprintw(win, y, 4, "( ) %s", options_[i].name.c_str());
                // In a real app we'd mark the current selection with (*)
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(CP_NORMAL));
                mvwprintw(win, y, 4, "( ) %s", options_[i].name.c_str());
                wattroff(win, COLOR_PAIR(CP_NORMAL));
            }

            // Description
            wattron(win, COLOR_PAIR(CP_NORMAL));
            mvwprintw(win, y + 1, 8, "%s", options_[i].description.c_str());
            wattroff(win, COLOR_PAIR(CP_NORMAL));
        }

        mvwprintw(win, h - 2, 2, "Use UP/DOWN to navigate, ENTER to select.");
    }

    bool handle_input(WINDOW* win, int ch) override {
        if (ch == KEY_UP && selected_ > 0) { selected_--; return true; }
        if (ch == KEY_DOWN && selected_ < (int)options_.size() - 1) { selected_++; return true; }
        if (ch == '\n' || ch == KEY_ENTER) {
            // Selection logic would go here
            return true;
        }
        return false;
    }
};
