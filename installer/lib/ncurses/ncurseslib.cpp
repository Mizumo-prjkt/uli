// ncurseslib.cpp - Core ncurses wrapper implementation for HarukaInstaller
#include "ncurseslib.hpp"
#include <clocale>

void NcursesLib::init_ncurses() {
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0); // hide cursor
  start_color();
  use_default_colors();

  // ── Define color pairs ──────────────────────────────────────────────
  // Main theme: blue background
  init_pair(CP_NORMAL, COLOR_WHITE, COLOR_BLUE);
  init_pair(CP_HIGHLIGHT, COLOR_BLUE, COLOR_WHITE);
  init_pair(CP_TITLE_BAR, COLOR_BLACK, COLOR_CYAN);
  init_pair(CP_STATUS_BAR, COLOR_BLACK, COLOR_CYAN);
  init_pair(CP_CONTENT_BORDER, COLOR_CYAN, COLOR_BLUE);
  init_pair(CP_INPUT_FIELD, COLOR_BLACK, COLOR_WHITE);
  init_pair(CP_SEPARATOR, COLOR_CYAN, COLOR_BLUE);
  init_pair(CP_ACTION_ITEM, COLOR_YELLOW, COLOR_BLUE);
  init_pair(CP_ACTION_HIGHLIGHT, COLOR_BLUE, COLOR_YELLOW);
  init_pair(CP_CHECKBOX_ON, COLOR_GREEN, COLOR_BLUE);
  init_pair(CP_CHECKBOX_OFF, COLOR_RED, COLOR_BLUE);
  init_pair(CP_SECTION_TITLE, COLOR_CYAN, COLOR_BLUE);
  init_pair(CP_TABLE_HEADER, COLOR_WHITE, COLOR_BLUE);
  init_pair(CP_TABLE_BORDER, COLOR_CYAN, COLOR_BLUE);

  // Help tile: black on cyan background
  init_pair(CP_HELP_WINDOW, COLOR_BLACK, COLOR_CYAN);
  init_pair(CP_HELP_TITLE, COLOR_BLACK, COLOR_CYAN);
  init_pair(CP_HELP_SEPARATOR, COLOR_BLACK, COLOR_CYAN);

  // Error popup colors
  init_pair(CP_ERROR_WINDOW, COLOR_WHITE, COLOR_RED);
  init_pair(CP_ERROR_TITLE, COLOR_WHITE, COLOR_RED);

  // Generic popup colors (Black on Cyan)
  init_pair(CP_POPUP_WINDOW, COLOR_BLACK, COLOR_CYAN);
  init_pair(CP_POPUP_TITLE, COLOR_BLACK, COLOR_CYAN);

  // Disk bar filesystem colors (colored text on colored background for blocks)
  init_pair(CP_DISK_EXT4, COLOR_BLACK, COLOR_GREEN);
  init_pair(CP_DISK_FAT32, COLOR_BLACK, COLOR_YELLOW);
  init_pair(CP_DISK_BTRFS, COLOR_BLACK, COLOR_CYAN);
  init_pair(CP_DISK_SWAP, COLOR_WHITE, COLOR_RED);
  init_pair(CP_DISK_XFS, COLOR_WHITE, COLOR_MAGENTA);
  init_pair(CP_DISK_NTFS, COLOR_BLACK, COLOR_WHITE);
  init_pair(CP_DISK_UNALLOC, COLOR_WHITE, COLOR_BLACK);

  // Fill stdscr with blue background
  bkgd(COLOR_PAIR(CP_NORMAL));
  refresh();
}

void NcursesLib::end_ncurses() { endwin(); }

void NcursesLib::draw_titled_box(WINDOW *win, const std::string &title) {
  int h, w;
  getmaxyx(win, h, w);
  (void)h;

  wattron(win, COLOR_PAIR(CP_CONTENT_BORDER));
  box(win, 0, 0);
  wattroff(win, COLOR_PAIR(CP_CONTENT_BORDER));

  if (!title.empty() && w > 4) {
    wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
    mvwprintw(win, 0, 2, " %s ", title.c_str());
    wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
  }
}

void NcursesLib::draw_hline(WINDOW *win, int y, int x1, int x2) {
  wattron(win, COLOR_PAIR(CP_SEPARATOR));
  mvwhline(win, y, x1, ACS_HLINE, x2 - x1 + 1);
  wattroff(win, COLOR_PAIR(CP_SEPARATOR));
}

void NcursesLib::fill_background(WINDOW *win, int color_pair) {
  wbkgd(win, COLOR_PAIR(color_pair));
  werase(win);
}

void NcursesLib::print_center(WINDOW *win, int y, const std::string &text) {
  int w = getmaxx(win);
  int x = (w - (int)text.size()) / 2;
  if (x < 0)
    x = 0;
  mvwprintw(win, y, x, "%s", text.c_str());
}

void NcursesLib::print_center_attr(WINDOW *win, int y, const std::string &text,
                                   int attrs) {
  wattron(win, attrs);
  print_center(win, y, text);
  wattroff(win, attrs);
}

std::string NcursesLib::text_input(WINDOW *win, int y, int x, int display_width, int max_len, const std::string& initial_value) {
    std::string res = initial_value;
    int cursor_pos = res.length();
    int offset = 0;
    curs_set(1);
    keypad(win, TRUE);
    
    auto redraw = [&]() {
        wattron(win, COLOR_PAIR(CP_INPUT_FIELD));
        for (int i = 0; i < display_width; i++) mvwaddch(win, y, x + i, ' ');
        
        std::string visible = res.substr(offset, display_width);
        mvwprintw(win, y, x, "%s", visible.c_str());
        wmove(win, y, x + (cursor_pos - offset));
        wattroff(win, COLOR_PAIR(CP_INPUT_FIELD));
        wrefresh(win);
    };

    while (true) {
        if (cursor_pos < offset) offset = cursor_pos;
        if (cursor_pos >= offset + display_width) offset = cursor_pos - display_width + 1;
        redraw();

        int ch = wgetch(win);
        if (ch == '\n' || ch == KEY_ENTER) break;
        if (ch == 27) return ""; // Cancel

        if (ch == KEY_LEFT && cursor_pos > 0) {
            cursor_pos--;
        } else if (ch == KEY_RIGHT && cursor_pos < (int)res.length()) {
            cursor_pos++;
        } else if (ch == KEY_HOME) {
            cursor_pos = 0;
        } else if (ch == KEY_END) {
            cursor_pos = res.length();
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (cursor_pos > 0) {
                res.erase(cursor_pos - 1, 1);
                cursor_pos--;
            }
        } else if (ch == KEY_DC) { // Delete
            if (cursor_pos < (int)res.length()) {
                res.erase(cursor_pos, 1);
            }
        } else if (isprint(ch) && (int)res.length() < max_len) {
            res.insert(cursor_pos, 1, (char)ch);
            cursor_pos++;
        }
    }
    curs_set(0);
    return res;
}

std::string NcursesLib::masked_input(WINDOW *win, int y, int x, int display_width, int max_len) {
    std::string res;
    int cursor_pos = 0;
    int offset = 0;
    curs_set(1);
    keypad(win, TRUE);

    auto redraw = [&]() {
        wattron(win, COLOR_PAIR(CP_INPUT_FIELD));
        for (int i = 0; i < display_width; i++) mvwaddch(win, y, x + i, ' ');
        
        for (int i = 0; i < std::min((int)res.length() - offset, display_width); i++) {
            mvwaddch(win, y, x + i, '*');
        }
        wmove(win, y, x + (cursor_pos - offset));
        wattroff(win, COLOR_PAIR(CP_INPUT_FIELD));
        wrefresh(win);
    };

    while (true) {
        if (cursor_pos < offset) offset = cursor_pos;
        if (cursor_pos >= offset + display_width) offset = cursor_pos - display_width + 1;
        redraw();

        int ch = wgetch(win);
        if (ch == '\n' || ch == KEY_ENTER) break;
        if (ch == 27) return "";

        if (ch == KEY_LEFT && cursor_pos > 0) {
            cursor_pos--;
        } else if (ch == KEY_RIGHT && cursor_pos < (int)res.length()) {
            cursor_pos++;
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (cursor_pos > 0) {
                res.erase(cursor_pos - 1, 1);
                cursor_pos--;
            }
        } else if (ch == KEY_DC) { // Delete
            if (cursor_pos < (int)res.length()) {
                res.erase(cursor_pos, 1);
            }
        } else if (isprint(ch) && (int)res.length() < max_len) {
            res.insert(cursor_pos, 1, (char)ch);
            cursor_pos++;
        }
    }
    curs_set(0);
    return res;
}
