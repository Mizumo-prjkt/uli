#pragma once
// popups.hpp - Reusable Popup Dialogs for HarukaInstaller
#include "ncurseslib.hpp"
#include <string>
#include <vector>
#include <algorithm>

class YesNoPopup {
public:
    static bool show(const std::string& title, const std::string& message) {
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        
        int h = 8;
        int w = std::max(40, (int)message.size() + 8);
        if (w > max_x - 4) w = max_x - 4;
        
        int y = (max_y - h) / 2;
        int x = (max_x - w) / 2;

        WINDOW* win = newwin(h, w, y, x);
        keypad(win, TRUE);

        NcursesLib::fill_background(win, CP_POPUP_WINDOW);

        int selected = 1; // Default to NO
        bool done = false;
        bool result = false;

        while (!done) {
            wattron(win, COLOR_PAIR(CP_POPUP_TITLE));
            box(win, 0, 0);

            wattron(win, A_BOLD);
            mvwprintw(win, 0, 2, " %s ", title.c_str());
            wattroff(win, A_BOLD);
            wattroff(win, COLOR_PAIR(CP_POPUP_TITLE));

            wattron(win, COLOR_PAIR(CP_POPUP_WINDOW));
            mvwprintw(win, 3, (w - message.size()) / 2, "%s", message.c_str());
            
            int yes_x = (w / 2) - 10;
            int no_x = (w / 2) + 4;
            
            if (selected == 0) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwprintw(win, h - 3, yes_x, "[ YES ]");
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                mvwprintw(win, h - 3, yes_x, "[ YES ]");
            }

            if (selected == 1) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwprintw(win, h - 3, no_x, "[ NO ]");
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                mvwprintw(win, h - 3, no_x, "[ NO ]");
            }
            wattroff(win, COLOR_PAIR(CP_POPUP_WINDOW));

            wrefresh(win);

            int ch = wgetch(win);
            if (ch == KEY_LEFT || ch == KEY_RIGHT) {
                selected = 1 - selected;
            } else if (ch == '\n' || ch == KEY_ENTER) {
                result = (selected == 0);
                done = true;
            } else if (NcursesLib::is_back_key(ch)) {
                result = false;
                done = true;
            }
        }

        delwin(win);
        touchwin(stdscr);
        return result;
    }
};

class InputPopup {
public:
    static std::string show(const std::string& title, const std::string& message, const std::string& initial = "") {
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        
        int h = 9;
        int w = std::max(50, (int)message.size() + 8);
        if (w > max_x - 4) w = max_x - 4;
        
        int y = (max_y - h) / 2;
        int x = (max_x - w) / 2;

        WINDOW* win = newwin(h, w, y, x);
        keypad(win, TRUE);

        NcursesLib::fill_background(win, CP_POPUP_WINDOW);

        wattron(win, COLOR_PAIR(CP_POPUP_TITLE));
        box(win, 0, 0);

        wattron(win, A_BOLD);
        mvwprintw(win, 0, 2, " %s ", title.c_str());
        wattroff(win, A_BOLD);
        wattroff(win, COLOR_PAIR(CP_POPUP_TITLE));

        wattron(win, COLOR_PAIR(CP_POPUP_WINDOW));
        mvwprintw(win, 3, (w - message.size()) / 2, "%s", message.c_str());
        wattroff(win, COLOR_PAIR(CP_POPUP_WINDOW));
        
        wrefresh(win);

        int input_w = w - 8;
        int input_x = 4;
        std::string result = NcursesLib::text_input(win, 5, input_x, input_w, initial);

        delwin(win);
        touchwin(stdscr);
        return result;
    }
};

enum class FieldType {
    Text,
    Boolean
};

struct FormField {
    std::string label;
    std::string comment;
    std::string value;
    int max_len = 50;
    FieldType type = FieldType::Text;
};

class FormPopup {
public:
    static bool show(const std::string& title, std::vector<FormField>& fields) {
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        
        int h = std::min(max_y - 4, 20);
        int w = std::min(max_x - 4, 70);
        
        int y = (max_y - h) / 2;
        int x = (max_x - w) / 2;

        WINDOW* win = newwin(h, w, y, x);
        keypad(win, TRUE);

        int selected = 0;
        int scroll = 0;
        bool done = false;
        bool saved = false;
        
        int total_items = fields.size() + 2;

        while (!done) {
            NcursesLib::fill_background(win, CP_POPUP_WINDOW);

            wattron(win, COLOR_PAIR(CP_POPUP_TITLE));
            box(win, 0, 0);
            wattron(win, A_BOLD);
            mvwprintw(win, 0, 2, " %s ", title.c_str());
            wattroff(win, A_BOLD);
            wattroff(win, COLOR_PAIR(CP_POPUP_TITLE));

            int content_h = h - 5;
            int item_spacing = 3; 
            
            if (selected < scroll) scroll = selected;
            if (selected >= scroll + (content_h / item_spacing) && selected < (int)fields.size()) {
                scroll = selected - (content_h / item_spacing) + 1;
            }

            int draw_y = 2;
            for (size_t i = scroll; i < fields.size() && draw_y < h - 4; i++) {
                wattron(win, COLOR_PAIR(CP_POPUP_WINDOW));
                mvwprintw(win, draw_y, 4, "%s:", fields[i].label.c_str());
                
                if (!fields[i].comment.empty()) {
                    wattron(win, A_DIM);
                    mvwprintw(win, draw_y + 1, 4, "%s", fields[i].comment.c_str());
                    wattroff(win, A_DIM);
                }
                
                if ((int)i == selected) {
                    wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                    if (fields[i].type == FieldType::Boolean) {
                        mvwprintw(win, draw_y, 4 + fields[i].label.size() + 2, " < %s > ", fields[i].value.c_str());
                    } else {
                        mvwprintw(win, draw_y, 4 + fields[i].label.size() + 2, " %-*s ", fields[i].max_len, fields[i].value.c_str());
                    }
                    wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
                } else {
                    if (fields[i].type == FieldType::Boolean) {
                        wattron(win, COLOR_PAIR(CP_POPUP_WINDOW));
                        mvwprintw(win, draw_y, 4 + fields[i].label.size() + 2, "   %s   ", fields[i].value.c_str());
                        wattroff(win, COLOR_PAIR(CP_POPUP_WINDOW));
                    } else {
                        wattron(win, COLOR_PAIR(CP_INPUT_FIELD));
                        mvwprintw(win, draw_y, 4 + fields[i].label.size() + 2, " %-*s ", fields[i].max_len, fields[i].value.c_str());
                        wattroff(win, COLOR_PAIR(CP_INPUT_FIELD));
                    }
                }
                wattroff(win, COLOR_PAIR(CP_POPUP_WINDOW));
                
                draw_y += item_spacing;
            }

            int save_x = w / 2 - 15;
            int cancel_x = w / 2 + 5;

            if (selected == (int)fields.size()) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwprintw(win, h - 2, save_x, "[ SAVE ]");
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(CP_POPUP_WINDOW));
                mvwprintw(win, h - 2, save_x, "[ SAVE ]");
                wattroff(win, COLOR_PAIR(CP_POPUP_WINDOW));
            }

            if (selected == (int)fields.size() + 1) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwprintw(win, h - 2, cancel_x, "[ CANCEL ]");
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(CP_POPUP_WINDOW));
                mvwprintw(win, h - 2, cancel_x, "[ CANCEL ]");
                wattroff(win, COLOR_PAIR(CP_POPUP_WINDOW));
            }

            wrefresh(win);

            int ch = wgetch(win);
            if (ch == KEY_UP && selected > 0) {
                selected--;
            } else if (ch == KEY_DOWN && selected < total_items - 1) {
                selected++;
            } else if (ch == KEY_LEFT || ch == KEY_RIGHT) {
                if (selected < (int)fields.size() && fields[selected].type == FieldType::Boolean) {
                    fields[selected].value = (fields[selected].value == "true") ? "false" : "true";
                } else if (selected == (int)fields.size() + 1 && ch == KEY_LEFT) {
                    selected--;
                } else if (selected == (int)fields.size() && ch == KEY_RIGHT) {
                    selected++;
                }
            } else if (ch == '\n' || ch == KEY_ENTER) {
                if (selected < (int)fields.size()) {
                    if (fields[selected].type == FieldType::Boolean) {
                        fields[selected].value = (fields[selected].value == "true") ? "false" : "true";
                    } else {
                        int idx_in_view = selected - scroll;
                        int field_y = 2 + (idx_in_view * item_spacing);
                        int field_x = 4 + fields[selected].label.size() + 2;
                        fields[selected].value = NcursesLib::text_input(win, field_y, field_x, fields[selected].max_len, fields[selected].value);
                        if (selected < (int)fields.size() - 1) selected++; 
                    }
                } else if (selected == (int)fields.size()) {
                    saved = true;
                    done = true;
                } else if (selected == (int)fields.size() + 1) {
                    saved = false;
                    done = true;
                }
            } else if (NcursesLib::is_back_key(ch)) {
                saved = false;
                done = true;
            }
        }

        delwin(win);
        touchwin(stdscr);
        return saved;
    }
};
