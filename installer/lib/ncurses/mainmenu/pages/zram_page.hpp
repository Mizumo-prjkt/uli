#pragma once
// zram_page.hpp - ZRAM and ZSWAP configuration page
#include "page.hpp"
#include "../../ncurseslib.hpp"
#include "../../configurations/datastore.hpp"

class ZramPage : public Page {
    int selected_ = 0; // 0=zram, 1=zswap

public:
    std::string title() const override { return "ZRAM and ZSWAP"; }

    void render(WINDOW* win) override {
        int h, w;
        getmaxyx(win, h, w);
        (void)h;

        auto& ds = DataStore::instance();

        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "ZRAM and ZSWAP Configuration");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        mvwprintw(win, 2, 2, "Toggle compressed RAM/swap features for your system.");

        NcursesLib::draw_hline(win, 3, 1, w - 2);

        // ZRAM
        if (selected_ == 0) {
            wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
            mvwhline(win, 5, 2, ' ', w - 4);
        }
        {
            int cp = ds.zram_enabled ? CP_CHECKBOX_ON : CP_CHECKBOX_OFF;
            const char* check = ds.zram_enabled ? "[x]" : "[ ]";
            if (selected_ != 0) wattron(win, COLOR_PAIR(cp));
            mvwprintw(win, 5, 3, "%s", check);
            if (selected_ != 0) wattroff(win, COLOR_PAIR(cp));
            if (selected_ == 0) {
                mvwprintw(win, 5, 7, "Enable ZRAM");
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(CP_NORMAL));
                mvwprintw(win, 5, 7, "Enable ZRAM");
                wattroff(win, COLOR_PAIR(CP_NORMAL));
            }
        }
        wattron(win, COLOR_PAIR(CP_SEPARATOR));
        mvwprintw(win, 6, 7, "ZRAM creates a compressed block device in RAM.");
        mvwprintw(win, 7, 7, "Useful as a faster swap alternative.");
        wattroff(win, COLOR_PAIR(CP_SEPARATOR));

        NcursesLib::draw_hline(win, 9, 1, w - 2);

        // ZSWAP
        if (selected_ == 1) {
            wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
            mvwhline(win, 11, 2, ' ', w - 4);
        }
        {
            int cp = ds.zswap_enabled ? CP_CHECKBOX_ON : CP_CHECKBOX_OFF;
            const char* check = ds.zswap_enabled ? "[x]" : "[ ]";
            if (selected_ != 1) wattron(win, COLOR_PAIR(cp));
            mvwprintw(win, 11, 3, "%s", check);
            if (selected_ != 1) wattroff(win, COLOR_PAIR(cp));
            if (selected_ == 1) {
                mvwprintw(win, 11, 7, "Enable ZSWAP");
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(CP_NORMAL));
                mvwprintw(win, 11, 7, "Enable ZSWAP");
                wattroff(win, COLOR_PAIR(CP_NORMAL));
            }
        }
        wattron(win, COLOR_PAIR(CP_SEPARATOR));
        mvwprintw(win, 12, 7, "ZSWAP compresses swap pages before writing to disk.");
        mvwprintw(win, 13, 7, "Reduces disk I/O at the cost of CPU usage.");
        wattroff(win, COLOR_PAIR(CP_SEPARATOR));
    }

    bool handle_input(WINDOW* win, int ch) override {
        (void)win;
        auto& ds = DataStore::instance();
        if (ch == KEY_UP && selected_ > 0) { selected_--; return true; }
        if (ch == KEY_DOWN && selected_ < 1) { selected_++; return true; }
        if (ch == ' ' || ch == '\n' || ch == KEY_ENTER) {
            if (selected_ == 0) ds.zram_enabled = !ds.zram_enabled;
            else ds.zswap_enabled = !ds.zswap_enabled;
            return true;
        }
        return false;
    }
};
