#pragma once
// profile_page.hpp - System Profile selection (Desktop/Server/Minimalist)
#include "page.hpp"
#include "../../ncurseslib.hpp"
#include "../../configurations/datastore.hpp"
#include <vector>
#include <string>

class ProfilePage : public Page {
    int selected_ = 0;

public:
    ProfilePage() {}

    std::string title() const override { return "System Profile"; }

    void render(WINDOW* win) override {
        int h, w;
        getmaxyx(win, h, w);
        auto& ds = DataStore::instance();

        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "System Profile");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        mvwprintw(win, 2, 2, "Select the type of system to install:");

        NcursesLib::draw_hline(win, 3, 1, w - 2);

        for (int i = 0; i < (int)ds.profiles.size() && i < h - 8; i++) {
            auto& p = ds.profiles[i];
            int y = 5 + i * 4; // Spacing for description

            if (i == selected_) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y, 2, ' ', w - 4);
                mvwprintw(win, y, 3, "%s  %s",
                    i == ds.selected_profile_idx ? "(*)" : "( )", p.name.c_str());
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                int cp = i == ds.selected_profile_idx ? CP_CHECKBOX_ON : CP_NORMAL;
                wattron(win, COLOR_PAIR(cp));
                mvwprintw(win, y, 3, "%s", i == ds.selected_profile_idx ? "(*)" : "( )");
                wattroff(win, COLOR_PAIR(cp));
                wattron(win, COLOR_PAIR(CP_NORMAL));
                mvwprintw(win, y, 7, "%s", p.name.c_str());
                wattroff(win, COLOR_PAIR(CP_NORMAL));
            }

            // Description below
            wattron(win, COLOR_PAIR(CP_SEPARATOR));
            mvwprintw(win, y + 1, 7, "%s", p.description.c_str());
            if (!p.default_de.empty())
                mvwprintw(win, y + 2, 7, "Default DE: %s", p.default_de.c_str());
            wattroff(win, COLOR_PAIR(CP_SEPARATOR));
        }
    }

    bool handle_input(WINDOW* win, int ch) override {
        (void)win;
        auto& ds = DataStore::instance();
        if (ch == KEY_UP && selected_ > 0) { selected_--; return true; }
        if (ch == KEY_DOWN && selected_ < (int)ds.profiles.size() - 1) { selected_++; return true; }
        if (ch == '\n' || ch == KEY_ENTER) { ds.selected_profile_idx = selected_; return true; }
        return false;
    }
};
