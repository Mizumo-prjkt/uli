// mainmenu.cpp - Main menu layout and event loop implementation
#include "mainmenu.hpp"
#include <algorithm>
#include <cstring>
#include "../configurations/datastore.hpp"

MainMenu::MainMenu() {}

MainMenu::~MainMenu() {
  for (auto &item : items_) {
    delete item.page;
  }
  destroy_windows();
}

void MainMenu::add_page(const std::string &label, Page *page) {
  items_.push_back({label, page, false});
}

void MainMenu::add_separator() { items_.push_back({"---", nullptr, false}); }

void MainMenu::add_action(const std::string &label) {
  items_.push_back({label, nullptr, true});
}

// ── Window Management ───────────────────────────────────────────────────────

void MainMenu::create_windows() {
  int rows = LINES;
  int cols = COLS;

  sidebar_width_ = std::max(28, cols / 4);

  // Title bar: row 0, full width
  win_title_ = newwin(1, cols, 0, 0);

  // Sidebar: rows 1 to LINES-3, left side
  int sidebar_h = rows - 3;
  win_sidebar_ = newwin(sidebar_h, sidebar_width_, 1, 0);

  // Content panel: rows 1 to LINES-3, right side
  int content_w = cols - sidebar_width_;
  win_content_ = newwin(sidebar_h, content_w, 1, sidebar_width_);

  // Status bar: last 2 rows
  win_status_ = newwin(2, cols, rows - 2, 0);

  // Enable keypad for all windows
  keypad(win_sidebar_, TRUE);
  keypad(win_content_, TRUE);

  // Set backgrounds
  NcursesLib::fill_background(win_title_, CP_TITLE_BAR);
  NcursesLib::fill_background(win_sidebar_, CP_NORMAL);
  NcursesLib::fill_background(win_content_, CP_NORMAL);
  NcursesLib::fill_background(win_status_, CP_STATUS_BAR);
}

void MainMenu::destroy_windows() {
  if (win_title_) {
    delwin(win_title_);
    win_title_ = nullptr;
  }
  if (win_sidebar_) {
    delwin(win_sidebar_);
    win_sidebar_ = nullptr;
  }
  if (win_content_) {
    delwin(win_content_);
    win_content_ = nullptr;
  }
  if (win_status_) {
    delwin(win_status_);
    win_status_ = nullptr;
  }
}

// ── Drawing ─────────────────────────────────────────────────────────────────

void MainMenu::draw_title_bar() {
  werase(win_title_);
  wattron(win_title_, COLOR_PAIR(CP_TITLE_BAR) | A_BOLD);
  mvwprintw(win_title_, 0, 1, "%s", Versioning::full_title().c_str());
  wattroff(win_title_, COLOR_PAIR(CP_TITLE_BAR) | A_BOLD);
  wrefresh(win_title_);
}

void MainMenu::draw_sidebar() {
  werase(win_sidebar_);
  int h = getmaxy(win_sidebar_);

  for (int i = 0; i < (int)items_.size() && i < h; i++) {
    auto &item = items_[i];

    // Separator
    if (item.label == "---") {
      NcursesLib::draw_hline(win_sidebar_, i, 0, sidebar_width_ - 1);
      continue;
    }

    bool is_selected = (i == sidebar_cursor_);
    bool focused_sel = is_selected && !content_focused_;

    if (item.is_action) {
      // Action items (Save/Install/Abort)
      if (focused_sel) {
        wattron(win_sidebar_, COLOR_PAIR(CP_ACTION_HIGHLIGHT) | A_BOLD);
        mvwhline(win_sidebar_, i, 0, ' ', sidebar_width_);
        mvwprintw(win_sidebar_, i, 1, "%s", item.label.c_str());
        wattroff(win_sidebar_, COLOR_PAIR(CP_ACTION_HIGHLIGHT) | A_BOLD);
      } else if (is_selected) {
        // Selected but content has focus — dim highlight
        wattron(win_sidebar_, COLOR_PAIR(CP_ACTION_ITEM) | A_BOLD);
        mvwprintw(win_sidebar_, i, 1, "%s", item.label.c_str());
        wattroff(win_sidebar_, COLOR_PAIR(CP_ACTION_ITEM) | A_BOLD);
      } else {
        wattron(win_sidebar_, COLOR_PAIR(CP_ACTION_ITEM));
        mvwprintw(win_sidebar_, i, 1, "%s", item.label.c_str());
        wattroff(win_sidebar_, COLOR_PAIR(CP_ACTION_ITEM));
      }
    } else {
      // Regular menu items
      if (focused_sel) {
        wattron(win_sidebar_, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
        mvwhline(win_sidebar_, i, 0, ' ', sidebar_width_);
        mvwprintw(win_sidebar_, i, 1, "%s", item.label.c_str());
        wattroff(win_sidebar_, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
      } else if (is_selected) {
        wattron(win_sidebar_, COLOR_PAIR(CP_HIGHLIGHT));
        mvwhline(win_sidebar_, i, 0, ' ', sidebar_width_);
        mvwprintw(win_sidebar_, i, 1, "%s", item.label.c_str());
        wattroff(win_sidebar_, COLOR_PAIR(CP_HIGHLIGHT));
      } else {
        wattron(win_sidebar_, COLOR_PAIR(CP_NORMAL));
        mvwprintw(win_sidebar_, i, 1, "%s", item.label.c_str());
        wattroff(win_sidebar_, COLOR_PAIR(CP_NORMAL));
      }
    }
  }

  wrefresh(win_sidebar_);
}

void MainMenu::draw_content() {
  werase(win_content_);

  auto &item = items_[sidebar_cursor_];
  if (item.page) {
    // Draw page title in a bordered content panel
    NcursesLib::draw_titled_box(win_content_, item.page->title());

    // Create a sub-window for the page content (inside the border)
    int ch, cw;
    getmaxyx(win_content_, ch, cw);
    WINDOW *inner = derwin(win_content_, ch - 2, cw - 2, 1, 1);
    NcursesLib::fill_background(inner, CP_NORMAL);

    item.page->render(inner);

    delwin(inner);
  } else if (item.is_action) {
    // Show action confirmation area
    NcursesLib::draw_titled_box(win_content_, item.label);
    int ch = getmaxy(win_content_);
    if (item.label == "INSTALL!") {
      NcursesLib::print_center_attr(win_content_, ch / 2 - 1,
                                    "Press ENTER to begin installation",
                                    COLOR_PAIR(CP_ACTION_ITEM) | A_BOLD);
      NcursesLib::print_center(
          win_content_, ch / 2 + 1,
          "(This is a test UI - no actual installation will occur)");
    } else if (item.label == "Save Config") {
      NcursesLib::print_center_attr(win_content_, ch / 2 - 1,
                                    "Press ENTER to save configuration",
                                    COLOR_PAIR(CP_ACTION_ITEM) | A_BOLD);
    } else if (item.label == "Abort") {
      NcursesLib::print_center_attr(win_content_, ch / 2 - 1,
                                    "Press ENTER to abort and exit",
                                    COLOR_PAIR(CP_CHECKBOX_OFF) | A_BOLD);
    }
  } else {
    // Separator selected (shouldn't happen)
    NcursesLib::print_center(win_content_, getmaxy(win_content_) / 2, "---");
  }

  wrefresh(win_content_);
}

void MainMenu::draw_status_bar() {
  werase(win_status_);
  int w = getmaxx(win_status_);

  wattron(win_status_, COLOR_PAIR(CP_STATUS_BAR));

  if (content_focused_) {
    // Content-focused hints
    mvwprintw(win_status_, 0, 1, "Mode:  [Content Configuration]");
    mvwprintw(win_status_, 1, 1, "Keys:  TAB (Cycle Sections)  ESC/LEFT/BACKSPACE (Back)  ARROWS (Navigate)");
  } else {
    // Sidebar-focused hints
    mvwprintw(win_status_, 0, 1, "Mode:  [Main Navigation]");
    mvwprintw(win_status_, 1, 1,
              "Keys:  ENTER/RIGHT (Select)  UP/DOWN (Navigate)  q (Quit)");
  }

  // Right-aligned help hints
  const char *help1 = "F1 - Quick Help";
  const char *help2 = "Shift+F1 - Wiki Search";
  mvwprintw(win_status_, 0, w - (int)strlen(help1) - 2, "%s", help1);
  mvwprintw(win_status_, 1, w - (int)strlen(help2) - 2, "%s", help2);

  wattroff(win_status_, COLOR_PAIR(CP_STATUS_BAR));
  wrefresh(win_status_);
}

// ── Navigation ──────────────────────────────────────────────────────────────

void MainMenu::cursor_up() {
  int prev = sidebar_cursor_;
  do {
    if (sidebar_cursor_ > 0)
      sidebar_cursor_--;
    else {
      sidebar_cursor_ = prev;
      return;
    }
  } while (items_[sidebar_cursor_].label == "---");
}

void MainMenu::cursor_down() {
  int prev = sidebar_cursor_;
  do {
    if (sidebar_cursor_ < (int)items_.size() - 1)
      sidebar_cursor_++;
    else {
      sidebar_cursor_ = prev;
      return;
    }
  } while (items_[sidebar_cursor_].label == "---");
}

void MainMenu::handle_action(const std::string &label) {
  if (label == "Abort") {
    werase(win_content_);
    NcursesLib::draw_titled_box(win_content_, "Abort");
    NcursesLib::print_center_attr(win_content_, getmaxy(win_content_) / 2,
                                  "Exiting HarukaInstaller...",
                                  COLOR_PAIR(CP_CHECKBOX_OFF) | A_BOLD);
    wrefresh(win_content_);
    napms(1000);
  } else if (label == "Save Config") {
    werase(win_content_);
    NcursesLib::draw_titled_box(win_content_, "Save Config");
    auto& ds = DataStore::instance();
    mvwprintw(win_content_, 4, 4, "Mirrors:    %zu", ds.mirrors.size());
    mvwprintw(win_content_, 5, 4, "Disks:      %zu", ds.disks.size());
    mvwprintw(win_content_, 6, 4, "ZRAM/ZSWAP: %s/%s", ds.zram_enabled ? "ON" : "OFF", ds.zswap_enabled ? "ON" : "OFF");
    NcursesLib::print_center_attr(win_content_, getmaxy(win_content_) - 4,
                                  "Configuration saved to DataStore!",
                                  COLOR_PAIR(CP_CHECKBOX_ON) | A_BOLD);
    wrefresh(win_content_);
    napms(1500);
  } else if (label == "INSTALL!") {
    werase(win_content_);
    NcursesLib::draw_titled_box(win_content_, "INSTALL!");
    NcursesLib::print_center_attr(win_content_, getmaxy(win_content_) / 2,
                                  "Installation would begin here! (test mode)",
                                  COLOR_PAIR(CP_ACTION_ITEM) | A_BOLD);
    wrefresh(win_content_);
    napms(2000);
  }
}

void MainMenu::handle_resize() {
  destroy_windows();
  endwin();
  refresh();
  create_windows();
}

void MainMenu::show_help() {
  // Show context-sensitive help based on current page
  auto &item = items_[sidebar_cursor_];
  std::string page_title = (item.page) ? item.label : "";
  HelpPopup::show(page_title);
}

// ── Main Event Loop ─────────────────────────────────────────────────────────

void MainMenu::run() {
  create_windows();

  // Initial draw
  draw_title_bar();
  draw_sidebar();
  draw_content();
  draw_status_bar();

  auto redraw_all_cb = [this]() {
    draw_title_bar();
    draw_sidebar();
    draw_content();
    draw_status_bar();
    touchwin(win_title_);
    touchwin(win_sidebar_);
    touchwin(win_content_);
    touchwin(win_status_);
    wrefresh(win_title_);
    wrefresh(win_sidebar_);
    wrefresh(win_content_);
    wrefresh(win_status_);
  };

  bool running = true;
  while (running) {
    // Get input from the appropriate window
    WINDOW *input_win = content_focused_ ? win_content_ : win_sidebar_;
    int ch = wgetch(input_win);

    if (ch == KEY_RESIZE) {
      handle_resize();
    } else if (!content_focused_) {
      // ── SIDEBAR MODE ────────────────────────────────────────────
      switch (ch) {
      case KEY_UP:
        cursor_up();
        break;
      case KEY_DOWN:
        cursor_down();
        break;
      case KEY_RIGHT:
      case '\t':
      case '\n':
      case KEY_ENTER:
        // Enter the selected page's content panel
        {
          auto &item = items_[sidebar_cursor_];
          if (item.is_action) {
            handle_action(item.label);
            if (item.label == "Abort") {
              running = false;
            }
          } else if (item.page) {
            content_focused_ = true;
          }
        }
        break;
      case 'q':
      case 'Q':
        running = false;
        break;
      case KEY_F(1):
        show_help();
        break;
      case KEY_F(13):
#ifdef KEY_SF1
      case KEY_SF1:
#endif
        HelpSearch::show(redraw_all_cb);
        break;
      default:
        break;
      }
    } else {
      // ── CONTENT MODE ────────────────────────────────────────────
      if (ch == KEY_F(1)) {
        show_help();
      } else if (ch == KEY_F(13)
#ifdef KEY_SF1
                 || ch == KEY_SF1
#endif
      ) {
        HelpSearch::show(redraw_all_cb);
      } else {
        // Forward ALL input to the page (including TAB, arrows, etc.)
        auto &item = items_[sidebar_cursor_];
        if (item.page) {
          int ph, pw;
          getmaxyx(win_content_, ph, pw);
          WINDOW *inner = derwin(win_content_, ph - 2, pw - 2, 1, 1);
          NcursesLib::fill_background(inner, CP_NORMAL);

          bool consumed = item.page->handle_input(inner, ch);
          delwin(inner);
          // Input stays contained — never leaks to sidebar unless not consumed
          if (!consumed && NcursesLib::is_back_key(ch)) {
            content_focused_ = false;
          }
        }
      }
    }

    // Redraw everything
    draw_title_bar();
    draw_sidebar();
    draw_content();
    draw_status_bar();
  }
}
