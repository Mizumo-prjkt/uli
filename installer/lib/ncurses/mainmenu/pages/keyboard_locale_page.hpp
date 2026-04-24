#pragma once
// keyboard_locale_page.hpp - Combined Keyboard layout and Locale configuration
#include "page.hpp"
#include "../../ncurseslib.hpp"
#include "../../configurations/datastore.hpp"
#include <vector>
#include <string>

class KeyboardLocalePage : public Page {
    int kb_selected_ = 0;
    int loc_selected_ = 0;
    int section_ = 0;   // 0 = keyboard, 1 = locale
    int kb_scroll_ = 0;
    int loc_scroll_ = 0;

public:
    KeyboardLocalePage() {}

    std::string title() const override { return "Keyboard and Locale"; }

    void render(WINDOW* win) override {
        int h, w;
        getmaxyx(win, h, w);
        auto& ds = DataStore::instance();

        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Keyboard and Locale Configuration");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        std::vector<std::pair<std::string, std::string>> options = {
            {"Keyboard Layout", ds.selected_kb_idx < (int)ds.keyboard_layouts.size() ? ds.keyboard_layouts[ds.selected_kb_idx].name : "None"},
            {"Locales", ds.selected_locales.empty() ? "None" : (ds.selected_locales[0] + (ds.selected_locales.size() > 1 ? " (+)" : ""))}
        };

        for (int i = 0; i < (int)options.size(); i++) {
            int y = 4 + (i * 2);
            if (i == kb_selected_) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y, 2, ' ', w - 4);
                mvwprintw(win, y, 4, "> %-20s : %s", options[i].first.c_str(), options[i].second.c_str());
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(CP_NORMAL));
                mvwprintw(win, y, 4, "  %-20s : %s", options[i].first.c_str(), options[i].second.c_str());
                wattroff(win, COLOR_PAIR(CP_NORMAL));
            }
        }

        wattron(win, A_DIM);
        mvwprintw(win, h - 2, 2, "Press ENTER to configure selected setting.");
        wattroff(win, A_DIM);
    }

    bool handle_input(WINDOW* win, int ch) override {
        (void)win;
        auto& ds = DataStore::instance();
        
        if (ch == KEY_UP && kb_selected_ > 0) { kb_selected_--; return true; }
        if (ch == KEY_DOWN && kb_selected_ < 1) { kb_selected_++; return true; }
        
        if (ch == '\n' || ch == KEY_ENTER) {
            if (kb_selected_ == 0) {
                // Keyboard Layout selection
                std::vector<ListOption> lopts;
                for (int i = 0; i < (int)ds.keyboard_layouts.size(); i++) {
                    lopts.push_back({ds.keyboard_layouts[i].name, ds.keyboard_layouts[i].name, i == ds.selected_kb_idx});
                }
                if (ListSelectPopup::show("Select Keyboard Layout", {"Choose your system keyboard layout."}, lopts, false, true)) {
                    for (int i = 0; i < (int)lopts.size(); i++) {
                        if (lopts[i].selected) {
                            ds.selected_kb_idx = i;
                            break;
                        }
                    }
                }
            } else {
                // Locale selection
                std::vector<ListOption> lopts;
                for (const auto& loc : ds.locales) {
                    bool sel = false;
                    for (const auto& s : ds.selected_locales) if (s == loc) sel = true;
                    lopts.push_back({loc, loc, sel});
                }
                if (ListSelectPopup::show("Select Locales", {"Select one or more locales for your system.", "Press SPACE to toggle, ENTER to save."}, lopts, true, true)) {
                    ds.selected_locales.clear();
                    for (const auto& lo : lopts) {
                        if (lo.selected) ds.selected_locales.push_back(lo.value);
                    }
                }
            }
            return true;
        }
        return false;
    }
};
