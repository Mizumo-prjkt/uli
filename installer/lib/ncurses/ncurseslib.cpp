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

std::string NcursesLib::text_input(WINDOW *win, int y, int x, int max_len, const std::string& initial_value) {
  std::string result = initial_value;
  curs_set(1);
  wattron(win, COLOR_PAIR(CP_INPUT_FIELD));
  // Draw input field background
  for (int i = 0; i < max_len; i++)
    mvwaddch(win, y, x + i, ' ');
  // Draw initial value
  mvwprintw(win, y, x, "%s", result.c_str());
  wmove(win, y, x + result.size());
  wrefresh(win);

  int ch;
  while ((ch = wgetch(win)) != '\n' && ch != 27) { // Enter or Escape
    if ((ch == KEY_BACKSPACE || ch == 127 || ch == 8) && !result.empty()) {
      result.pop_back();
      mvwaddch(win, y, x + (int)result.size(), ' ');
      wmove(win, y, x + (int)result.size());
    } else if (ch >= 32 && ch < 127 && (int)result.size() < max_len) {
      result += (char)ch;
      mvwaddch(win, y, x + (int)result.size() - 1, ch);
    }
    wrefresh(win);
  }
  wattroff(win, COLOR_PAIR(CP_INPUT_FIELD));
  curs_set(0);
  if (ch == 27)
    return ""; // cancelled
  return result;
}

std::string NcursesLib::masked_input(WINDOW *win, int y, int x, int max_len) {
  std::string result;
  curs_set(1);
  wattron(win, COLOR_PAIR(CP_INPUT_FIELD));
  for (int i = 0; i < max_len; i++)
    mvwaddch(win, y, x + i, ' ');
  wmove(win, y, x);
  wrefresh(win);

  int ch;
  while ((ch = wgetch(win)) != '\n' && ch != 27) {
    if ((ch == KEY_BACKSPACE || ch == 127 || ch == 8) && !result.empty()) {
      result.pop_back();
      mvwaddch(win, y, x + (int)result.size(), ' ');
      wmove(win, y, x + (int)result.size());
    } else if (ch >= 32 && ch < 127 && (int)result.size() < max_len) {
      result += (char)ch;
      mvwaddch(win, y, x + (int)result.size() - 1, '*');
    }
    wrefresh(win);
  }
  wattroff(win, COLOR_PAIR(CP_INPUT_FIELD));
  curs_set(0);
  if (ch == 27)
    return "";
  return result;
}
