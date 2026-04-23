#pragma once
// error_popup.hpp - Generic Error Popup Window
#include "ncurseslib.hpp"
#include <algorithm>
#include <string>

class ErrorPopup {
public:
  static void show(const std::string &title, const std::string &message) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    int h = 8;
    int w = std::max(40, (int)message.size() + 8);
    if (w > max_x - 4)
      w = max_x - 4;

    int y = (max_y - h) / 2;
    int x = (max_x - w) / 2;

    WINDOW *win = newwin(h, w, y, x);
    keypad(win, TRUE);

    NcursesLib::fill_background(win, CP_ERROR_WINDOW);

    // Draw border
    wattron(win, COLOR_PAIR(CP_ERROR_TITLE));
    box(win, 0, 0);

    // Draw title
    wattron(win, A_BOLD);
    mvwprintw(win, 0, 2, " %s ", title.c_str());
    wattroff(win, A_BOLD);
    wattroff(win, COLOR_PAIR(CP_ERROR_TITLE));

    // Draw message
    wattron(win, COLOR_PAIR(CP_ERROR_WINDOW));
    mvwprintw(win, 3, (w - message.size()) / 2, "%s", message.c_str());
    wattroff(win, COLOR_PAIR(CP_ERROR_WINDOW));

    // Action hint
    wattron(win, COLOR_PAIR(CP_ERROR_WINDOW) | A_DIM);
    mvwprintw(win, h - 2, (w - 18) / 2, "[ Press any key ]");
    wattroff(win, COLOR_PAIR(CP_ERROR_WINDOW) | A_DIM);

    wrefresh(win);
    wgetch(win);
    delwin(win);

    // Touch stdscr so the caller's next refresh redraws the background
    touchwin(stdscr);
  }
};
