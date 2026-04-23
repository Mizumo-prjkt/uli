#pragma once
// help_search.hpp - Wiki Search and Guide Popup
#include "../ncurseslib.hpp"
#include "helpdocs/helpdocs.hpp"
#include "help.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

class HelpSearch {
public:
    static void show(std::function<void()> redraw_bg = nullptr) {
        int h = LINES - 6;
        int w = COLS - 10;
        int y = (LINES - h) / 2;
        int x = (COLS - w) / 2;

        WINDOW* win = newwin(h, w, y, x);
        keypad(win, TRUE);
        NcursesLib::fill_background(win, CP_HELP_WINDOW);

        std::string query = "";
        auto all_entries = HelpDocs::wiki_entries();
        std::vector<std::pair<std::string, HelpDocs::HelpEntry>> filtered;
        
        // Initial populate
        for (auto const& [key, val] : all_entries) filtered.push_back({key, val});

        int selected = 0;
        bool running = true;

        while (running) {
            werase(win);
            NcursesLib::fill_background(win, CP_HELP_WINDOW);
            wattron(win, COLOR_PAIR(CP_HELP_WINDOW));
            box(win, 0, 0);
            wattroff(win, COLOR_PAIR(CP_HELP_WINDOW));

            wattron(win, COLOR_PAIR(CP_HELP_TITLE) | A_BOLD);
            mvwprintw(win, 0, (w - 20) / 2, " Wiki Help Search ");
            wattroff(win, COLOR_PAIR(CP_HELP_TITLE) | A_BOLD);

            // Search bar
            mvwprintw(win, 1, 2, "Search: ");
            wattron(win, COLOR_PAIR(CP_INPUT_FIELD));
            mvwprintw(win, 1, 10, " %-30s ", query.c_str());
            wattroff(win, COLOR_PAIR(CP_INPUT_FIELD));

            wattron(win, COLOR_PAIR(CP_HELP_SEPARATOR));
            mvwhline(win, 2, 1, ACS_HLINE, w - 2);
            wattroff(win, COLOR_PAIR(CP_HELP_SEPARATOR));

            // Results
            int list_h = h - 5;
            for (int i = 0; i < (int)filtered.size() && i < list_h; i++) {
                int ry = 3 + i;
                if (i == selected) {
                    wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                    mvwhline(win, ry, 2, ' ', w - 4);
                    mvwprintw(win, ry, 3, "> %s", filtered[i].first.c_str());
                    wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
                } else {
                    wattron(win, COLOR_PAIR(CP_HELP_WINDOW));
                    mvwprintw(win, ry, 3, "  %s", filtered[i].first.c_str());
                    wattroff(win, COLOR_PAIR(CP_HELP_WINDOW));
                }
            }

            mvwprintw(win, h - 2, 2, "ESC=Back  ENTER=View  TAB=Toggle Search");
            wrefresh(win);

            int ch = wgetch(win);
            if (ch == 27) running = false; // ESC
            else if (ch == KEY_UP && selected > 0) selected--;
            else if (ch == KEY_DOWN && selected < (int)filtered.size() - 1) selected++;
            else if (ch == '\n' || ch == KEY_ENTER) {
                if (!filtered.empty()) {
                    // Show guide popup
                    HelpPopup::show_direct(filtered[selected].second);
                    // Redraw background to clear the popup's ghost
                    if (redraw_bg) redraw_bg();
                    // Re-touch after returning
                    touchwin(win);
                }
            } else if (ch == '\t') {
                // Interactive search
                curs_set(1);
                std::string res = NcursesLib::text_input(win, 1, 11, 28);
                curs_set(0);
                query = res;
                // Filter
                filtered.clear();
                std::string lquery = query;
                std::transform(lquery.begin(), lquery.end(), lquery.begin(), ::tolower);
                for (auto const& [key, val] : all_entries) {
                    std::string lkey = key;
                    std::transform(lkey.begin(), lkey.end(), lkey.begin(), ::tolower);
                    if (lkey.find(lquery) != std::string::npos) {
                        filtered.push_back({key, val});
                    }
                }
                selected = 0;
            }
        }

        delwin(win);
    }
};
