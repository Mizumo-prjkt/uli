#pragma once
// kernels_page.hpp - Kernel selection page with multi-select checkboxes
#include "page.hpp"
#include "../../ncurseslib.hpp"
#include "../../configurations/datastore.hpp"
#include <vector>
#include <string>

class KernelsPage : public Page {
    int cursor_ = 0;

public:
    KernelsPage() {}

    std::string title() const override { return "Kernel Selection"; }

    void render(WINDOW* win) override {
        int h, w;
        getmaxyx(win, h, w);
        auto& ds = DataStore::instance();

        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Kernel Selection");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        mvwprintw(win, 2, 2, "Use SPACE to toggle, at least one kernel must be selected.");

        NcursesLib::draw_hline(win, 3, 1, w - 2);

        for (int i = 0; i < (int)ds.kernels.size() && i < h - 6; i++) {
            auto& k = ds.kernels[i];
            int y = 4 + i * 2; // Double-spaced for readability

            if (i == cursor_) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y, 2, ' ', w - 4);
            }

            int check_cp = k.selected ? CP_CHECKBOX_ON : CP_CHECKBOX_OFF;
            const char* check = k.selected ? "[x]" : "[ ]";

            if (i != cursor_) wattron(win, COLOR_PAIR(check_cp));
            mvwprintw(win, y, 3, "%s", check);
            if (i != cursor_) wattroff(win, COLOR_PAIR(check_cp));

            if (i == cursor_) {
                mvwprintw(win, y, 7, "%-20s  %s", k.package.c_str(), k.description.c_str());
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(CP_NORMAL));
                mvwprintw(win, y, 7, "%-20s", k.package.c_str());
                wattroff(win, COLOR_PAIR(CP_NORMAL));
                wattron(win, COLOR_PAIR(CP_SEPARATOR));
                mvwprintw(win, y, 29, "%s", k.description.c_str());
                wattroff(win, COLOR_PAIR(CP_SEPARATOR));
            }
        }

        // Summary at bottom
        int sum_y = h - 3;
        NcursesLib::draw_hline(win, sum_y - 1, 1, w - 2);
        mvwprintw(win, sum_y, 2, "Selected: ");
        wattron(win, COLOR_PAIR(CP_CHECKBOX_ON) | A_BOLD);
        int sx = 12;
        for (auto& k : ds.kernels) {
            if (k.selected) {
                mvwprintw(win, sum_y, sx, "%s  ", k.package.c_str());
                sx += (int)k.package.size() + 2;
            }
        }
        wattroff(win, COLOR_PAIR(CP_CHECKBOX_ON) | A_BOLD);
    }

    bool handle_input(WINDOW* win, int ch) override {
        (void)win;
        auto& ds = DataStore::instance();
        if (ch == KEY_UP && cursor_ > 0) { cursor_--; return true; }
        if (ch == KEY_DOWN && cursor_ < (int)ds.kernels.size() - 1) { cursor_++; return true; }
        if (ch == ' ' || ch == '\n' || ch == KEY_ENTER) {
            // Don't allow deselecting if it's the only selected kernel
            if (ds.kernels[cursor_].selected) {
                int count = 0;
                for (auto& k : ds.kernels) if (k.selected) count++;
                if (count <= 1) return true; // can't deselect last one
            }
            ds.kernels[cursor_].selected = !ds.kernels[cursor_].selected;
            return true;
        }
        return false;
    }
};
