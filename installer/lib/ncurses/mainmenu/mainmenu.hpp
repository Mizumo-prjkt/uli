#pragma once
// mainmenu.hpp - Main menu layout and event loop for HarukaInstaller TUI
// Manages the title bar, sidebar navigation, content panel, and status bar.

#include "../ncurseslib.hpp"
#include "../help/help.hpp"
#include "../help/help_search.hpp"
#include "../versioning/versioning.hpp"
#include "pages/page.hpp"
#include <vector>
#include <string>
#include <memory>

// Sidebar menu item
struct MenuItem {
    std::string label;
    Page*       page;     // owned by MainMenu
    bool        is_action; // true for Save/Install/Abort group
};

class MainMenu {
public:
    MainMenu();
    ~MainMenu();

    // Add a page to the menu. MainMenu takes ownership of the Page pointer.
    void add_page(const std::string& label, Page* page);

    // Add a separator in the sidebar
    void add_separator();

    // Add an action item (Save Config, INSTALL!, Abort)
    void add_action(const std::string& label);

    // Run the main event loop. Returns when user selects Abort or presses 'q'.
    void run();

private:
    std::vector<MenuItem> items_;
    int sidebar_cursor_ = 0;
    bool content_focused_ = false;

    // Windows
    WINDOW* win_title_   = nullptr;
    WINDOW* win_sidebar_ = nullptr;
    WINDOW* win_content_ = nullptr;
    WINDOW* win_status_  = nullptr;

    int sidebar_width_ = 28;

    void create_windows();
    void destroy_windows();
    void draw_title_bar();
    void draw_sidebar();
    void draw_content();
    void draw_status_bar();
    void handle_resize();
    void handle_action(const std::string& label);

    // Skip separator items
    void cursor_up();
    void cursor_down();

    // Help popup (? key)
    void show_help();
};
