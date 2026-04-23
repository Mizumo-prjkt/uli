#pragma once
// language_page.hpp - Installer Language selection page
#include "page.hpp"
#include "../../ncurseslib.hpp"
#include "../../configurations/datastore.hpp"
#include <vector>
#include <string>

class LanguagePage : public Page {
    int selected_ = 0;
    int scroll_ = 0;

public:
    LanguagePage() {}

    std::string title() const override { return "Installer Language"; }

    void render(WINDOW* win) override {
        int h, w;
        getmaxyx(win, h, w);
        auto& ds = DataStore::instance();

        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Select Installer Language");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        if (ds.selected_language_idx < (int)ds.languages.size()) {
            mvwprintw(win, 2, 2, "Current: %s", ds.languages[ds.selected_language_idx].name.c_str());
        }

        NcursesLib::draw_hline(win, 3, 1, w - 2);

        int list_h = h - 6;
        if (selected_ < scroll_) scroll_ = selected_;
        if (selected_ >= scroll_ + list_h) scroll_ = selected_ - list_h + 1;

        for (int i = 0; i < list_h && (scroll_ + i) < (int)ds.languages.size(); i++) {
            int idx = scroll_ + i;
            int y = 4 + i;
            if (idx == selected_) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y, 2, ' ', w - 4);
                mvwprintw(win, y, 3, "%s %s",
                    idx == ds.selected_language_idx ? "*" : " ",
                    ds.languages[idx].name.c_str());
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(CP_NORMAL));
                mvwprintw(win, y, 3, "%s %s",
                    idx == ds.selected_language_idx ? "*" : " ",
                    ds.languages[idx].name.c_str());
                wattroff(win, COLOR_PAIR(CP_NORMAL));
            }
        }
    }

    bool handle_input(WINDOW* win, int ch) override {
        (void)win;
        auto& ds = DataStore::instance();
        if (ch == KEY_UP && selected_ > 0) { selected_--; return true; }
        if (ch == KEY_DOWN && selected_ < (int)ds.languages.size() - 1) { selected_++; return true; }
        if (ch == '\n' || ch == KEY_ENTER) { ds.selected_language_idx = selected_; return true; }
        return false;
    }
};
