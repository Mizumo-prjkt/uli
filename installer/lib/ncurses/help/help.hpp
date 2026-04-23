#pragma once
// help.hpp - DOS-style popup help window for HarukaInstaller TUI
#include "../ncurseslib.hpp"
#include "helpdocs/helpdocs.hpp"
#include <sstream>
#include <string>
#include <vector>

class HelpPopup {
public:
  // Show a centered help popup. Blocks until user presses any key.
  static void show(const std::string &page_title = "") {
    // Get the right help content
    HelpDocs::HelpEntry entry;
    auto pages = HelpDocs::page_help();
    auto it = pages.find(page_title);
    if (it != pages.end()) {
      entry = it->second;
    } else {
      entry = HelpDocs::general();
    }
    show_direct(entry);
  }

  // Directly show a specific help entry
  static void show_direct(const HelpDocs::HelpEntry &entry) {
    // Split body into lines
    std::vector<std::string> lines;
    std::istringstream stream(entry.body);
    std::string line;
    int max_line = 0;
    while (std::getline(stream, line)) {
      lines.push_back(line);
      if ((int)line.size() > max_line)
        max_line = (int)line.size();
    }

    // Calculate popup dimensions
    int title_len = (int)entry.title.size();
    if (title_len + 4 > max_line)
      max_line = title_len + 4;

    int popup_w = max_line + 6;          // 3 margin each side
    int popup_h = (int)lines.size() + 4; // 2 border + 1 title line + 1 bottom

    // Clamp to screen
    if (popup_w > COLS - 4)
      popup_w = COLS - 4;
    if (popup_h > LINES - 4)
      popup_h = LINES - 4;

    int popup_y = (LINES - popup_h) / 2;
    int popup_x = (COLS - popup_w) / 2;

    // Create popup window
    WINDOW *popup = newwin(popup_h, popup_w, popup_y, popup_x);
    keypad(popup, TRUE);
    NcursesLib::fill_background(popup, CP_HELP_WINDOW);

    // Draw border with title
    wattron(popup, COLOR_PAIR(CP_HELP_WINDOW));
    box(popup, 0, 0);
    // Double-line top border for DOS feel
    mvwhline(popup, 0, 1, ACS_HLINE, popup_w - 2);
    mvwaddch(popup, 0, 0, ACS_ULCORNER);
    mvwaddch(popup, 0, popup_w - 1, ACS_URCORNER);
    wattroff(popup, COLOR_PAIR(CP_HELP_WINDOW));

    // Title
    wattron(popup, COLOR_PAIR(CP_HELP_TITLE) | A_BOLD);
    int tx = (popup_w - (int)entry.title.size() - 4) / 2;
    if (tx < 1)
      tx = 1;
    mvwprintw(popup, 0, tx, " %s ", entry.title.c_str());
    wattroff(popup, COLOR_PAIR(CP_HELP_TITLE) | A_BOLD);

    // Body text with scrolling support
    int body_h = popup_h - 2;
    int scroll = 0;
    int total = (int)lines.size();
    bool scrollable = total > body_h;

    // Render and wait loop (for scrolling)
    int ch;
    do {
      // Clear body area
      for (int i = 1; i < popup_h - 1; i++) {
        wmove(popup, i, 1);
        wclrtoeol(popup);
        // Redraw right border
        mvwaddch(popup, i, popup_w - 1, ACS_VLINE);
      }
      NcursesLib::fill_background(popup, CP_HELP_WINDOW);
      wattron(popup, COLOR_PAIR(CP_HELP_WINDOW));
      box(popup, 0, 0);
      wattroff(popup, COLOR_PAIR(CP_HELP_WINDOW));
      wattron(popup, COLOR_PAIR(CP_HELP_TITLE) | A_BOLD);
      mvwprintw(popup, 0, tx, " %s ", entry.title.c_str());
      wattroff(popup, COLOR_PAIR(CP_HELP_TITLE) | A_BOLD);

      // Draw body
      wattron(popup, COLOR_PAIR(CP_HELP_WINDOW));
      for (int i = 0; i < body_h && (scroll + i) < total; i++) {
        std::string &l = lines[scroll + i];
        // Truncate if too long
        if ((int)l.size() > popup_w - 4)
          mvwprintw(popup, 1 + i, 2, "%.*s", popup_w - 4, l.c_str());
        else
          mvwprintw(popup, 1 + i, 2, "%s", l.c_str());
      }
      wattroff(popup, COLOR_PAIR(CP_HELP_WINDOW));

      // Scroll indicators
      if (scrollable) {
        if (scroll > 0) {
          wattron(popup, COLOR_PAIR(CP_HELP_TITLE));
          mvwprintw(popup, 1, popup_w - 2, "^");
          wattroff(popup, COLOR_PAIR(CP_HELP_TITLE));
        }
        if (scroll + body_h < total) {
          wattron(popup, COLOR_PAIR(CP_HELP_TITLE));
          mvwprintw(popup, popup_h - 2, popup_w - 2, "v");
          wattroff(popup, COLOR_PAIR(CP_HELP_TITLE));
        }
      }

      wrefresh(popup);
      ch = wgetch(popup);

      if (scrollable) {
        if (ch == KEY_UP && scroll > 0) {
          scroll--;
          continue;
        }
        if (ch == KEY_DOWN && scroll + body_h < total) {
          scroll++;
          continue;
        }
        if (ch == KEY_PPAGE) {
          scroll -= body_h;
          if (scroll < 0)
            scroll = 0;
          continue;
        }
        if (ch == KEY_NPAGE) {
          scroll += body_h;
          if (scroll + body_h > total)
            scroll = total - body_h;
          continue;
        }
      }
      break; // Any other key closes
    } while (true);

    delwin(popup);
    // Force full redraw after popup
    // UPD: Let the caller handle redrawing the background
    // touchwin(stdscr);
    // refresh();
  }
};
