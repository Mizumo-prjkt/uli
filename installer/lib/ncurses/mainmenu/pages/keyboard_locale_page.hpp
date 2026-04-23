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
        int half_h = (h - 4) / 2;
        auto& ds = DataStore::instance();

        // ── Keyboard section ──
        int kb_cp = section_ == 0 ? CP_SECTION_TITLE : CP_SEPARATOR;
        wattron(win, COLOR_PAIR(kb_cp) | A_BOLD);
        mvwprintw(win, 1, 2, "Keyboard Layout");
        wattroff(win, COLOR_PAIR(kb_cp) | A_BOLD);
        
        if (ds.selected_kb_idx < (int)ds.keyboard_layouts.size()) {
            mvwprintw(win, 1, 20, " (Current: %s)", ds.keyboard_layouts[ds.selected_kb_idx].name.c_str());
        }

        int kb_list_h = half_h - 2;
        if (kb_selected_ < kb_scroll_) kb_scroll_ = kb_selected_;
        if (kb_selected_ >= kb_scroll_ + kb_list_h) kb_scroll_ = kb_selected_ - kb_list_h + 1;

        for (int i = 0; i < kb_list_h && (kb_scroll_ + i) < (int)ds.keyboard_layouts.size(); i++) {
            int idx = kb_scroll_ + i;
            int y = 2 + i;
            if (section_ == 0 && idx == kb_selected_) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y, 2, ' ', w - 4);
                mvwprintw(win, y, 3, "%s %s",
                    idx == ds.selected_kb_idx ? "*" : " ", ds.keyboard_layouts[idx].name.c_str());
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(CP_NORMAL));
                mvwprintw(win, y, 3, "%s %s",
                    idx == ds.selected_kb_idx ? "*" : " ", ds.keyboard_layouts[idx].name.c_str());
                wattroff(win, COLOR_PAIR(CP_NORMAL));
            }
        }

        // ── Separator ──
        int sep_y = 2 + half_h - 1;
        NcursesLib::draw_hline(win, sep_y, 1, w - 2);

        // ── Locale section ──
        int loc_cp = section_ == 1 ? CP_SECTION_TITLE : CP_SEPARATOR;
        wattron(win, COLOR_PAIR(loc_cp) | A_BOLD);
        mvwprintw(win, sep_y + 1, 2, "Locale");
        wattroff(win, COLOR_PAIR(loc_cp) | A_BOLD);
        
        int loc_confirmed_idx = -1; // For now just use a simple state or similar
        // Actually we should probably have a single confirmed locale for the UI
        mvwprintw(win, sep_y + 1, 10, " (Current: %s)", ds.locales.empty() ? "None" : ds.locales[0].c_str()); 

        int loc_start = sep_y + 2;
        int loc_list_h = h - loc_start - 1;
        if (loc_selected_ < loc_scroll_) loc_scroll_ = loc_selected_;
        if (loc_selected_ >= loc_scroll_ + loc_list_h) loc_scroll_ = loc_selected_ - loc_list_h + 1;

        for (int i = 0; i < loc_list_h && (loc_scroll_ + i) < (int)ds.locales.size(); i++) {
            int idx = loc_scroll_ + i;
            int y = loc_start + i;
            bool is_active = false;
            for (const auto& sel : ds.selected_locales) if (sel == ds.locales[idx]) is_active = true;

            if (section_ == 1 && idx == loc_selected_) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y, 2, ' ', w - 4);
                mvwprintw(win, y, 3, "%s %s",
                    is_active ? "*" : " ", ds.locales[idx].c_str());
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(CP_NORMAL));
                mvwprintw(win, y, 3, "%s %s",
                    is_active ? "*" : " ", ds.locales[idx].c_str());
                wattroff(win, COLOR_PAIR(CP_NORMAL));
            }
        }
    }

    bool handle_input(WINDOW* win, int ch) override {
        (void)win;
        auto& ds = DataStore::instance();
        if (ch == '\t' || ch == KEY_BTAB) {
            section_ = 1 - section_;
            return true;
        }
        if (section_ == 0) {
            if (ch == KEY_UP && kb_selected_ > 0) { kb_selected_--; return true; }
            if (ch == KEY_DOWN && kb_selected_ < (int)ds.keyboard_layouts.size() - 1) { kb_selected_++; return true; }
            if (ch == '\n' || ch == KEY_ENTER) { ds.selected_kb_idx = kb_selected_; return true; }
        } else {
            if (ch == KEY_UP && loc_selected_ > 0) { loc_selected_--; return true; }
            if (ch == KEY_DOWN && loc_selected_ < (int)ds.locales.size() - 1) { loc_selected_++; return true; }
            if (ch == '\n' || ch == KEY_ENTER) {
                std::string loc = ds.locales[loc_selected_];
                auto it = std::find(ds.selected_locales.begin(), ds.selected_locales.end(), loc);
                if (it == ds.selected_locales.end()) ds.selected_locales.push_back(loc);
                else ds.selected_locales.erase(it);
                return true;
            }
        }
        return false;
    }
};
