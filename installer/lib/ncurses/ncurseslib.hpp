#pragma once
// ncurseslib.hpp - Core ncurses wrapper for HarukaInstaller
// Provides initialization, color management, and drawing utilities.

#include <ncurses.h>
#include <string>
#include <vector>

// ── Color Pair IDs ──────────────────────────────────────────────────────────
enum ColorPair {
    CP_NORMAL = 1,       // White on blue  (default text)
    CP_HIGHLIGHT,        // Blue on white   (selected menu item)
    CP_TITLE_BAR,        // Bold white on blue (title bar)
    CP_STATUS_BAR,       // Black on cyan   (key-hints bar)
    CP_CONTENT_BORDER,   // Cyan on blue    (content panel border)
    CP_INPUT_FIELD,      // Black on white  (text input fields)
    CP_SEPARATOR,        // Dim cyan on blue (separator lines)
    CP_ACTION_ITEM,      // Yellow on blue  (Save/Install/Abort)
    CP_ACTION_HIGHLIGHT, // Blue on yellow  (selected action)
    CP_CHECKBOX_ON,      // Green on blue   (enabled checkbox)
    CP_CHECKBOX_OFF,     // Red on blue     (disabled checkbox)
    CP_SECTION_TITLE,    // Bold cyan on blue (section headers)
    CP_TABLE_HEADER,     // Bold white on blue
    CP_TABLE_BORDER,     // Cyan on blue
    // Help popup colors
    CP_HELP_WINDOW,      // Black on cyan
    CP_HELP_TITLE,       // Black on cyan (banner style)
    CP_HELP_SEPARATOR,   // Black on cyan line
    // Error popup colors
    CP_ERROR_WINDOW,     // White on red
    CP_ERROR_TITLE,      // White on red (banner style)
    // Generic popups
    CP_POPUP_WINDOW,
    CP_POPUP_TITLE,
    // Disk bar colors
    CP_DISK_EXT4,        // Green
    CP_DISK_FAT32,       // Yellow
    CP_DISK_BTRFS,       // Cyan
    CP_DISK_SWAP,        // Red
    CP_DISK_XFS,         // Magenta
    CP_DISK_NTFS,        // White
    CP_DISK_UNALLOC,     // Dark/black
};

class NcursesLib {
public:
    // Lifecycle
    void init_ncurses();
    void end_ncurses();

    // Drawing utilities (static so pages can use them freely)
    static void draw_titled_box(WINDOW* win, const std::string& title);
    static void draw_hline(WINDOW* win, int y, int x1, int x2);
    static void fill_background(WINDOW* win, int color_pair);
    static void print_center(WINDOW* win, int y, const std::string& text);
    static void print_center_attr(WINDOW* win, int y, const std::string& text, int attrs);

    // Input helpers
    static std::string text_input(WINDOW* win, int y, int x, int display_width, int max_len, const std::string& initial_value = "");
    static std::string masked_input(WINDOW* win, int y, int x, int display_width, int max_len);
    
    static bool is_back_key(int ch) {
        return (ch == 27 || ch == KEY_BACKSPACE || ch == 127 || ch == 8);
    }
};

class ScrollableList {
public:
    std::vector<std::string> items;
    int selected_idx = 0;
    int scroll_offset = 0;
    bool is_focused = false;
    std::string list_name;

    ScrollableList(const std::string& name) : list_name(name) {}

    void render(WINDOW* win, int y, int x, int width, int height) {
        if (is_focused) {
            wattron(win, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
            mvwprintw(win, y - 1, x, "[ %s (Scrollable) ]", list_name.c_str());
            wattroff(win, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
        } else {
            wattron(win, COLOR_PAIR(CP_NORMAL) | A_BOLD);
            mvwprintw(win, y - 1, x, "%s", list_name.c_str());
            wattroff(win, COLOR_PAIR(CP_NORMAL) | A_BOLD);
        }

        if (items.empty()) {
            mvwprintw(win, y, x, "  (Empty List)");
            return;
        }

        if (selected_idx < scroll_offset) scroll_offset = selected_idx;
        if (selected_idx >= scroll_offset + height) scroll_offset = selected_idx - height + 1;

        for (int i = 0; i < height && (i + scroll_offset) < (int)items.size(); i++) {
            int idx = scroll_offset + i;
            if (is_focused && idx == selected_idx) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y + i, x, ' ', width);
                mvwprintw(win, y + i, x, "> %.*s", width - 2, items[idx].c_str());
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(CP_NORMAL));
                mvwhline(win, y + i, x, ' ', width);
                mvwprintw(win, y + i, x, "  %.*s", width - 2, items[idx].c_str());
                wattroff(win, COLOR_PAIR(CP_NORMAL));
            }
        }

        // Draw scrollbar indicator
        if ((int)items.size() > height) {
            int bar_x = x + width;
            for (int i = 0; i < height; i++) {
                mvwaddch(win, y + i, bar_x, ACS_VLINE | COLOR_PAIR(CP_CONTENT_BORDER));
            }
            float progress = (float)scroll_offset / (items.size() - height);
            int thumb_y = y + (int)(progress * (height - 1));
            wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
            mvwaddch(win, thumb_y, bar_x, ACS_DIAMOND);
            wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
        }
    }

    bool handle_input(int ch) {
        if (!is_focused || items.empty()) return false;
        if (ch == KEY_UP && selected_idx > 0) {
            selected_idx--;
            return true;
        }
        if (ch == KEY_DOWN && selected_idx < (int)items.size() - 1) {
            selected_idx++;
            return true;
        }
        return false;
    }
};

