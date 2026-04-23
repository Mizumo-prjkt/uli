#pragma once
// stub_page.hpp - Placeholder page for not-yet-implemented sections
#include "page.hpp"
#include "../../ncurseslib.hpp"
#include <string>

class StubPage : public Page {
    std::string title_;

public:
    StubPage(const std::string& page_title) : title_(page_title) {}

    std::string title() const override { return title_; }

    void render(WINDOW* win) override {
        int h, w;
        getmaxyx(win, h, w);

        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "%s", title_.c_str());
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        NcursesLib::draw_hline(win, 3, 1, w - 2);

        NcursesLib::print_center_attr(win, h / 2 - 1,
            "[ Coming Soon ]", COLOR_PAIR(CP_SEPARATOR) | A_BOLD);
        NcursesLib::print_center(win, h / 2 + 1,
            "This section is not yet implemented.");
    }

    bool handle_input(WINDOW* win, int ch) override {
        (void)win; (void)ch;
        return false;
    }
};
