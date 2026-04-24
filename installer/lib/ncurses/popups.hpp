#pragma once
// popups.hpp - Reusable Popup Dialogs for HarukaInstaller
#include "ncurseslib.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <functional>
#include "mainmenu/isolatebuffer.hpp"

struct ListOption {
    std::string value;
    std::string label;
    bool selected = false;
};

class ListSelectPopup {
public:
    static bool show(
        const std::string& title, 
        const std::vector<std::string>& instructions, 
        std::vector<ListOption>& options, 
        bool multi_select, 
        bool enable_search
    ) {
        IsolateBuffer iso; // Capture screen state
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        
        int h = std::min(max_y - 4, 25);
        int w = std::min(max_x - 4, 80);
        int y = (max_y - h) / 2;
        int x = (max_x - w) / 2;

        WINDOW* win = newwin(h, w, y, x);
        keypad(win, TRUE);

        std::string search_query = "";
        int selected = enable_search ? -1 : 0; // -1 means search box is focused
        int scroll = 0;
        bool done = false;
        bool saved = false;

        while (!done) {
            std::vector<int> filtered_indices;
            for (size_t i = 0; i < options.size(); i++) {
                if (search_query.empty()) {
                    filtered_indices.push_back(i);
                } else {
                    std::string q = search_query;
                    std::transform(q.begin(), q.end(), q.begin(), ::tolower);
                    std::string l = options[i].label;
                    std::transform(l.begin(), l.end(), l.begin(), ::tolower);
                    std::string v = options[i].value;
                    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
                    if (l.find(q) != std::string::npos || v.find(q) != std::string::npos) {
                        filtered_indices.push_back(i);
                    }
                }
            }

            NcursesLib::fill_background(win, CP_POPUP_WINDOW);

            wattron(win, COLOR_PAIR(CP_POPUP_TITLE));
            box(win, 0, 0);
            wattron(win, A_BOLD);
            mvwprintw(win, 0, 2, " %s ", title.c_str());
            wattroff(win, A_BOLD);
            wattroff(win, COLOR_PAIR(CP_POPUP_TITLE));

            int curr_y = 2;
            if (!instructions.empty()) {
                wattron(win, A_DIM);
                for (const auto& line : instructions) {
                    mvwprintw(win, curr_y++, 4, "%s", line.c_str());
                }
                wattroff(win, A_DIM);
                curr_y++;
            }

            if (enable_search) {
                mvwprintw(win, curr_y, 4, "Search: ");
                if (selected == -1) {
                    wattron(win, COLOR_PAIR(CP_INPUT_FIELD));
                    mvwprintw(win, curr_y, 12, " %-*s ", w - 18, search_query.c_str());
                    wattroff(win, COLOR_PAIR(CP_INPUT_FIELD));
                } else {
                    wattron(win, COLOR_PAIR(CP_POPUP_WINDOW) | A_UNDERLINE);
                    mvwprintw(win, curr_y, 12, " %-*s ", w - 18, search_query.c_str());
                    wattroff(win, COLOR_PAIR(CP_POPUP_WINDOW) | A_UNDERLINE);
                }
                curr_y += 2;
            }

            int list_start_y = curr_y;
            int list_h = h - list_start_y - 3;
            
            if (selected >= (int)filtered_indices.size() + 2) {
                selected = filtered_indices.size() + 1;
            }

            if (selected >= 0) {
                if (selected < scroll) scroll = selected;
                if (selected >= scroll + list_h) scroll = selected - list_h + 1;
            } else {
                scroll = 0; // if search is focused
            }

            for (int i = 0; i < list_h && scroll + i < (int)filtered_indices.size(); i++) {
                int idx = scroll + i;
                int opt_idx = filtered_indices[idx];
                bool is_sel = (idx == selected);
                
                int draw_y = list_start_y + i;
                if (is_sel) {
                    wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                    mvwhline(win, draw_y, 2, ' ', w - 4);
                }
                
                char checkbox = options[opt_idx].selected ? (multi_select ? 'x' : '*') : ' ';
                mvwprintw(win, draw_y, 4, "[%c] %s (%s)", checkbox, options[opt_idx].label.c_str(), options[opt_idx].value.c_str());
                
                if (is_sel) {
                    wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
                }
            }

            if (filtered_indices.empty()) {
                wattron(win, A_DIM);
                mvwprintw(win, list_start_y, 4, "No results found.");
                wattroff(win, A_DIM);
            }

            int save_x = w / 2 - 12;
            int cancel_x = w / 2 + 2;

            int btn_selected_idx = filtered_indices.size();
            
            if (selected == btn_selected_idx) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwprintw(win, h - 2, save_x, "[ SAVE ]");
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                mvwprintw(win, h - 2, save_x, "[ SAVE ]");
            }

            if (selected == btn_selected_idx + 1) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwprintw(win, h - 2, cancel_x, "[ CANCEL ]");
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                mvwprintw(win, h - 2, cancel_x, "[ CANCEL ]");
            }

            wrefresh(win);

            int ch = wgetch(win);
            
            if (ch == KEY_UP) {
                if (selected > (enable_search ? -1 : 0)) selected--;
            } else if (ch == KEY_DOWN) {
                if (selected < btn_selected_idx + 1) selected++;
            } else if (ch == '\t') {
                 if (selected == -1 && !filtered_indices.empty()) selected = 0;
                 else if (selected >= 0 && selected < btn_selected_idx) selected = btn_selected_idx;
                 else if (selected >= btn_selected_idx) selected = enable_search ? -1 : 0;
            } else if (selected == -1) {
                // Search box input
                if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
                    if (!search_query.empty()) search_query.pop_back();
                } else if (isprint(ch)) {
                    search_query += (char)ch;
                } else if (ch == '\n' || ch == KEY_ENTER) {
                    if (!filtered_indices.empty()) selected = 0;
                } else if (NcursesLib::is_back_key(ch)) {
                    saved = false;
                    done = true;
                }
            } else if (selected >= 0 && selected < btn_selected_idx) {
                // List items input
                if (ch == ' ' || ch == '\n' || ch == KEY_ENTER) {
                    int opt_idx = filtered_indices[selected];
                    if (multi_select) {
                        options[opt_idx].selected = !options[opt_idx].selected;
                    } else {
                        for (auto& o : options) o.selected = false;
                        options[opt_idx].selected = true;
                        if (ch == '\n' || ch == KEY_ENTER) {
                            saved = true;
                            done = true;
                        }
                    }
                } else if (NcursesLib::is_back_key(ch)) {
                    saved = false;
                    done = true;
                } else if (enable_search && isprint(ch)) {
                    search_query += (char)ch;
                    selected = -1; // switch back to search
                } else if (enable_search && (ch == KEY_BACKSPACE || ch == 127 || ch == '\b')) {
                    if (!search_query.empty()) search_query.pop_back();
                    selected = -1;
                }
            } else if (selected == btn_selected_idx) { // SAVE
                if (ch == '\n' || ch == KEY_ENTER) {
                    saved = true;
                    done = true;
                } else if (ch == KEY_RIGHT) {
                    selected++;
                } else if (ch == KEY_LEFT && !filtered_indices.empty()) {
                    selected = filtered_indices.size() - 1;
                } else if (NcursesLib::is_back_key(ch)) {
                    saved = false;
                    done = true;
                }
            } else if (selected == btn_selected_idx + 1) { // CANCEL
                if (ch == '\n' || ch == KEY_ENTER) {
                    saved = false;
                    done = true;
                } else if (ch == KEY_LEFT) {
                    selected--;
                } else if (NcursesLib::is_back_key(ch)) {
                    saved = false;
                    done = true;
                }
            } else {
                if (NcursesLib::is_back_key(ch)) {
                    saved = false;
                    done = true;
                }
            }
        }

        delwin(win);
        touchwin(stdscr);
        return saved;
    }
};

class YesNoPopup {
public:
    static bool show(const std::string& title, const std::string& message, const std::string& note = "") {
        IsolateBuffer iso;
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        
        // Calculate dimensions
        std::vector<std::string> msg_lines;
        std::string tmp;
        std::stringstream ss(message);
        while (std::getline(ss, tmp, '\n')) msg_lines.push_back(tmp);

        std::vector<std::string> note_lines;
        if (!note.empty()) {
            std::stringstream ss_note(note);
            while (std::getline(ss_note, tmp, '\n')) note_lines.push_back(tmp);
        }

        int h = 6 + msg_lines.size() + (note.empty() ? 0 : note_lines.size() + 1);
        int max_w = 40;
        for (const auto& l : msg_lines) max_w = std::max(max_w, (int)l.size() + 8);
        for (const auto& l : note_lines) max_w = std::max(max_w, (int)l.size() + 8);
        int w = std::min(max_w, max_x - 4);
        
        int y = (max_y - h) / 2;
        int x = (max_x - w) / 2;

        WINDOW* win = newwin(h, w, y, x);
        keypad(win, TRUE);

        int selected = 1; // Default to NO
        bool done = false;
        bool result = false;

        while (!done) {
            NcursesLib::fill_background(win, CP_POPUP_WINDOW);
            wattron(win, COLOR_PAIR(CP_POPUP_TITLE));
            box(win, 0, 0);
            wattron(win, A_BOLD);
            mvwprintw(win, 0, 2, " %s ", title.c_str());
            wattroff(win, A_BOLD);
            wattroff(win, COLOR_PAIR(CP_POPUP_TITLE));

            wattron(win, COLOR_PAIR(CP_POPUP_WINDOW));
            int curr_y = 2;
            for (const auto& line : msg_lines) {
                mvwprintw(win, curr_y++, (w - line.size()) / 2, "%s", line.c_str());
            }
            
            if (!note.empty()) {
                curr_y++; // Gap
                wattron(win, A_DIM);
                for (const auto& line : note_lines) {
                    mvwprintw(win, curr_y++, (w - line.size()) / 2, "%s", line.c_str());
                }
                wattroff(win, A_DIM);
            }
            
            int yes_x = (w / 2) - 10;
            int no_x = (w / 2) + 4;
            int btn_y = h - 2;
            
            if (selected == 0) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwprintw(win, btn_y, yes_x, "[ YES ]");
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                mvwprintw(win, btn_y, yes_x, "[ YES ]");
            }

            if (selected == 1) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwprintw(win, btn_y, no_x, "[ NO ]");
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                mvwprintw(win, btn_y, no_x, "[ NO ]");
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
        IsolateBuffer iso;
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
    Boolean,
    Select
};

struct FormField {
    std::string label;
    std::string comment;
    std::string value;
    int max_len = 50;
    FieldType type = FieldType::Text;
    std::vector<std::string> options; // For Select type
    bool hidden = false;
};

class FormPopup {
public:
    static bool show(const std::string& title, std::vector<FormField>& fields, 
                    std::function<void(std::vector<FormField>&)> update_fn = nullptr) {
        IsolateBuffer iso; // Capture screen state
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        
        int h = std::min(max_y - 4, 20);
        int w = std::min(max_x - 4, 80);
        
        int y = (max_y - h) / 2;
        int x = (max_x - w) / 2;

        WINDOW* win = newwin(h, w, y, x);
        keypad(win, TRUE);

        int selected = 0;
        int scroll = 0;
        bool done = false;
        bool saved = false;
        
        while (!done) {
            if (update_fn) update_fn(fields);
            
            std::vector<int> visible_indices;
            for (int i = 0; i < (int)fields.size(); i++) {
                if (!fields[i].hidden) visible_indices.push_back(i);
            }
            int total_items = visible_indices.size() + 2;

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
            if (selected >= scroll + (content_h / item_spacing) && selected < (int)visible_indices.size()) {
                scroll = selected - (content_h / item_spacing) + 1;
            }

            int draw_y = 2;
            for (size_t vi = scroll; vi < visible_indices.size() && draw_y < h - 4; vi++) {
                int i = visible_indices[vi];
                wattron(win, COLOR_PAIR(CP_POPUP_WINDOW));
                mvwprintw(win, draw_y, 4, "%s:", fields[i].label.c_str());
                
                if (!fields[i].comment.empty()) {
                    wattron(win, A_DIM);
                    mvwprintw(win, draw_y + 1, 4, "%s", fields[i].comment.c_str());
                    wattroff(win, A_DIM);
                }
                
                if ((int)vi == selected) {
                    wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                    if (fields[i].type == FieldType::Boolean) {
                        mvwprintw(win, draw_y, 4 + fields[i].label.size() + 2, " < %s > ", fields[i].value.c_str());
                    } else if (fields[i].type == FieldType::Select) {
                        mvwprintw(win, draw_y, 4 + fields[i].label.size() + 2, " [ %s ] ", fields[i].value.c_str());
                    } else {
                        mvwprintw(win, draw_y, 4 + fields[i].label.size() + 2, " %-*s ", fields[i].max_len, fields[i].value.c_str());
                    }
                    wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
                } else {
                    if (fields[i].type == FieldType::Boolean || fields[i].type == FieldType::Select) {
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

            if (selected == (int)visible_indices.size()) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwprintw(win, h - 2, save_x, "[ SAVE ]");
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                wattron(win, COLOR_PAIR(CP_POPUP_WINDOW));
                mvwprintw(win, h - 2, save_x, "[ SAVE ]");
                wattroff(win, COLOR_PAIR(CP_POPUP_WINDOW));
            }

            if (selected == (int)visible_indices.size() + 1) {
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
            if (NcursesLib::is_back_key(ch)) {
                done = true;
            } else if (ch == KEY_UP && selected > 0) {
                selected--;
            } else if (ch == KEY_DOWN && selected < total_items - 1) {
                selected++;
            } else if (ch == KEY_LEFT || ch == KEY_RIGHT) {
                if (selected < (int)visible_indices.size()) {
                    int i = visible_indices[selected];
                    if (fields[i].type == FieldType::Boolean) {
                        fields[i].value = (fields[i].value == "true") ? "false" : "true";
                    }
                } else if (selected == (int)visible_indices.size() + 1 && ch == KEY_LEFT) {
                    selected--;
                } else if (selected == (int)visible_indices.size() && ch == KEY_RIGHT) {
                    selected++;
                }
            } else if (ch == '\n' || ch == KEY_ENTER) {
                if (selected < (int)visible_indices.size()) {
                    int i = visible_indices[selected];
                    if (fields[i].type == FieldType::Boolean) {
                        fields[i].value = (fields[i].value == "true") ? "false" : "true";
                    } else if (fields[i].type == FieldType::Select) {
                        std::vector<ListOption> lopts;
                        for (const auto& o : fields[i].options) {
                            lopts.push_back({o, o, o == fields[i].value});
                        }
                        if (ListSelectPopup::show("Select " + fields[i].label, {}, lopts, false, false)) {
                            for (const auto& lo : lopts) {
                                if (lo.selected) {
                                    fields[i].value = lo.value;
                                    break;
                                }
                            }
                        }
                    } else {
                        int idx_in_view = selected - scroll;
                        int field_y = 2 + (idx_in_view * item_spacing);
                        int field_x = 4 + fields[i].label.size() + 2;
                        fields[i].value = NcursesLib::text_input(win, field_y, field_x, fields[i].max_len, fields[i].value);
                        if (selected < (int)visible_indices.size() - 1) selected++; 
                    }
                } else if (selected == (int)visible_indices.size()) {
                    saved = true;
                    done = true;
                } else if (selected == (int)visible_indices.size() + 1) {
                    done = true;
                }
            }
        }

        delwin(win);
        touchwin(stdscr);
        return saved;
    }
};
