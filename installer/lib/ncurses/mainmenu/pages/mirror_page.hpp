#pragma once
// mirror_page.hpp - Mirror repository configuration page
#include "page.hpp"
#include "../../ncurseslib.hpp"
#include "../../error_popup.hpp"
#include "../../popups.hpp"
#include "../../configurations/datastore.hpp"
#include <vector>
#include <string>
#include <algorithm>

enum class MirrorSection {
    Main,
    Reflector,
    Manual,
    Settings
};

class MirrorPage : public Page {
    MirrorSection section_ = MirrorSection::Main;
    int selected_ = 0;
    
    // Sub-section states
    std::string manual_url_ = "";
    ScrollableList manual_list_{"Added Mirrors"};

public:
    MirrorPage() {}

    std::string title() const override { 
        switch(section_) {
            case MirrorSection::Reflector: return "Mirrors > Auto Fetch";
            case MirrorSection::Manual:    return "Mirrors > Manual Input";
            case MirrorSection::Settings:  return "Mirrors > Pacman Settings";
            default:                       return "Mirror Configuration";
        }
    }

    void render(WINDOW* win) override {
        switch(section_) {
            case MirrorSection::Main:      render_main(win); break;
            case MirrorSection::Reflector: render_reflector(win); break;
            case MirrorSection::Manual:    render_manual(win); break;
            case MirrorSection::Settings:  render_settings(win); break;
        }
    }

    bool handle_input(WINDOW* win, int ch) override {
        // Modular back keys go back to Main section
        if (NcursesLib::is_back_key(ch)) {
            // Intercept to only unfocus the list if we're in the Manual section list mode
            if (section_ == MirrorSection::Manual && manual_list_.is_focused) {
                manual_list_.is_focused = false;
                return true;
            }

            if (section_ != MirrorSection::Main) {
                // Discard pending changes when backing out
                if (section_ == MirrorSection::Manual) {
                    manual_list_.items.clear();
                    manual_list_.is_focused = false;
                }
                section_ = MirrorSection::Main;
                selected_ = 0;
                return true;
            }
            return false; // Let MainMenu handle the exit to sidebar
        }

        switch(section_) {
            case MirrorSection::Main:      return handle_main(ch);
            case MirrorSection::Reflector: return handle_reflector(ch);
            case MirrorSection::Manual:    return handle_manual(win, ch);
            case MirrorSection::Settings:  return handle_settings(ch);
        }
        return false;
    }

private:
    bool is_valid_protocol(const std::string& url) {
        return (url.find("https://") == 0 || url.find("http://") == 0 || 
                url.find("ftp://") == 0 || url.find("rsync://") == 0);
    }

    std::string normalize_url(std::string url) {
        // Strip any existing variables to prevent duplication or malformed strings
        size_t pos = url.find("$repo");
        if (pos != std::string::npos) url = url.substr(0, pos);
        
        pos = url.find("$arch");
        if (pos != std::string::npos) url = url.substr(0, pos);
        
        // Strip trailing slashes and dangling "os" directory
        while (!url.empty() && url.back() == '/') url.pop_back();
        if (url.length() >= 3 && url.substr(url.length() - 3) == "/os") {
            url = url.substr(0, url.length() - 3);
        }
        
        if (!url.empty() && url.back() != '/') url += "/";
        url += "$repo/os/$arch";
        
        return url;
    }
    void render_main(WINDOW* win) {
        int h, w;
        getmaxyx(win, h, w);

        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Mirror Configuration");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        mvwprintw(win, 2, 2, "Select how you want to configure your repositories:");
        NcursesLib::draw_hline(win, 3, 1, w - 2);

        const char* options[] = {
            "Auto fetch possible server points (using reflector)",
            "Manual Input of Repo",
            "Configure Pacman Settings"
        };

        for (int i = 0; i < 3; i++) {
            int y = 5 + i * 2;
            if (i == selected_) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y, 2, ' ', w - 4);
                mvwprintw(win, y, 4, "> %s", options[i]);
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                mvwprintw(win, y, 4, "  %s", options[i]);
            }
        }
    }

    void render_reflector(WINDOW* win) {
        int h, w;
        getmaxyx(win, h, w);

        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Auto Fetch (Reflector)");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        mvwprintw(win, 3, 4, "Countries:      %s", DataStore::instance().reflector_cfg.countries.c_str());
        mvwprintw(win, 5, 4, "Number (Last):  %d", DataStore::instance().reflector_cfg.last_n);
        mvwprintw(win, 7, 4, "Sort Method:    %s", DataStore::instance().reflector_cfg.sort.c_str());

        const char* actions[] = { "Update Countries", "Change Number", "Change Sort", "[ RUN REFLECTOR ]", "[ SAVE & BACK ]", "[ DO NOT SAVE & BACK ]" };
        for (int i = 0; i < 6; i++) {
            int y = 10 + i;
            if (i == selected_) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y, 4, ' ', w - 8);
                mvwprintw(win, y, 6, "%s", actions[i]);
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                mvwprintw(win, y, 6, "%s", actions[i]);
            }
        }
    }

    void render_manual(WINDOW* win) {
        int h, w;
        getmaxyx(win, h, w);

        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Manual Repo Input");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        mvwprintw(win, 3, 4, "Enter Repository URL:");
        wattron(win, COLOR_PAIR(CP_INPUT_FIELD));
        mvwprintw(win, 4, 4, " %-50s ", manual_url_.c_str());
        wattroff(win, COLOR_PAIR(CP_INPUT_FIELD));

        // Disclaimer (stacked below input)
        wattron(win, A_DIM);
        mvwprintw(win, 5, 4, "Note: Protocols (https, ftp, etc.) are required.");
        mvwprintw(win, 6, 4, "$repo/os/$arch is added if missing.");
        wattroff(win, A_DIM);

        // Render Scrollable List
        manual_list_.render(win, 9, 4, w - 8, 5);

        // Action Options
        const char* actions[] = { "Edit URL", "Add to List", "Clear All", "Manage List", "[ SAVE & BACK ]", "[ DO NOT SAVE & BACK ]" };
        int action_y_start = 16;
        for (int i = 0; i < 6; i++) {
            int y = action_y_start + i;
            if (!manual_list_.is_focused && i == selected_) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y, 4, ' ', w - 8);
                mvwprintw(win, y, 6, "%s", actions[i]);
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                mvwprintw(win, y, 6, "%s", actions[i]);
            }
        }

        // Help hint for list focus
        if (manual_list_.is_focused) {
            wattron(win, A_DIM | COLOR_PAIR(CP_STATUS_BAR));
            mvwprintw(win, h - 2, 2, "[UP/DOWN: Scroll] [E: Edit] [D: Delete] [ESC: Exit List]");
            wattroff(win, A_DIM | COLOR_PAIR(CP_STATUS_BAR));
        }
    }

    void render_settings(WINDOW* win) {
        int h, w;
        getmaxyx(win, h, w);

        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Pacman Configuration");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        auto draw_toggle = [&](int y, const char* label, bool val, int idx) {
            if (idx == selected_) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y, 4, ' ', w - 8);
                mvwprintw(win, y, 6, "[%c] %s", val ? 'x' : ' ', label);
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                mvwprintw(win, y, 6, "[%c] %s", val ? 'x' : ' ', label);
            }
        };

        draw_toggle(4, "Enable Parallel Downloads", DataStore::instance().pacman_cfg.parallel_downloads, 0);
        draw_toggle(5, "Enable Color Output", DataStore::instance().pacman_cfg.color, 1);
        draw_toggle(6, "Check Disk Space", DataStore::instance().pacman_cfg.check_space, 2);
        draw_toggle(7, "Verbose Package Lists", DataStore::instance().pacman_cfg.verbose_pkg_lists, 3);

        const char* extras[] = { "[ SAVE & BACK ]", "[ DO NOT SAVE & BACK ]" };
        for (int i = 0; i < 2; i++) {
            int y = 9 + i;
            int idx = 4 + i;
            if (idx == selected_) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwhline(win, y, 4, ' ', w - 8);
                mvwprintw(win, y, 6, "%s", extras[i]);
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                mvwprintw(win, y, 6, "%s", extras[i]);
            }
        }
    }

    bool handle_main(int ch) {
        if (ch == KEY_UP && selected_ > 0) { selected_--; return true; }
        if (ch == KEY_DOWN && selected_ < 2) { selected_++; return true; }
        if (ch == '\n' || ch == KEY_ENTER) {
            if (selected_ == 0) section_ = MirrorSection::Reflector;
            else if (selected_ == 1) section_ = MirrorSection::Manual;
            else if (selected_ == 2) section_ = MirrorSection::Settings;
            selected_ = 0;
            return true;
        }
        return false;
    }

    bool handle_reflector(int ch) {
        if (ch == KEY_UP && selected_ > 0) { selected_--; return true; }
        if (ch == KEY_DOWN && selected_ < 5) { selected_++; return true; }
        if (ch == '\n' || ch == KEY_ENTER) {
            if (selected_ >= 0 && selected_ <= 2) {
                std::vector<FormField> fields = {
                    {"Countries", "Comma separated list of countries", DataStore::instance().reflector_cfg.countries, 50, FieldType::Text},
                    {"Last N", "Number of mirrors to test", std::to_string(DataStore::instance().reflector_cfg.last_n), 5, FieldType::Text},
                    {"Sort", "rate, age, country, score, delay", DataStore::instance().reflector_cfg.sort, 10, FieldType::Text}
                };
                if (FormPopup::show("Reflector Configuration", fields)) {
                    DataStore::instance().reflector_cfg.countries = fields[0].value;
                    try { DataStore::instance().reflector_cfg.last_n = std::stoi(fields[1].value); } catch(...) {}
                    DataStore::instance().reflector_cfg.sort = fields[2].value;
                }
            } else if (selected_ == 3) {
                // RUN: In a real app, this would show a progress popup
            } else if (selected_ == 4 || selected_ == 5) { 
                section_ = MirrorSection::Main; 
                selected_ = 0; 
            }
            return true;
        }
        return false;
    }

    bool handle_manual(WINDOW* win, int ch) {
        if (manual_list_.is_focused) {
            if (ch == 'e' || ch == 'E' || ch == '\n' || ch == KEY_ENTER) {
                if (!manual_list_.items.empty()) {
                    std::string current_val = manual_list_.items[manual_list_.selected_idx];
                    std::string edited = InputPopup::show("Edit Mirror", "Repository URL:", current_val);
                    if (!edited.empty()) {
                        if (is_valid_protocol(edited)) manual_list_.items[manual_list_.selected_idx] = normalize_url(edited);
                        else ErrorPopup::show("Validation Error", "Invalid protocol (https/ftp/rsync required)");
                    }
                }
                return true;
            }
            if (ch == 'd' || ch == 'D' || ch == KEY_DC) {
                if (!manual_list_.items.empty()) {
                    if (YesNoPopup::show("Confirm Delete", "Are you sure you want to remove this mirror?")) {
                        manual_list_.items.erase(manual_list_.items.begin() + manual_list_.selected_idx);
                        if (manual_list_.selected_idx > 0 && manual_list_.selected_idx >= (int)manual_list_.items.size()) {
                            manual_list_.selected_idx--;
                        }
                    }
                }
                return true;
            }
            return manual_list_.handle_input(ch);
        }

        if (ch == KEY_UP && selected_ > 0) { selected_--; return true; }
        if (ch == KEY_DOWN && selected_ < 5) { selected_++; return true; }
        if (ch == '\n' || ch == KEY_ENTER) {
            if (selected_ == 0) {
                manual_url_ = InputPopup::show("Manual Input", "Enter Repository URL:", manual_url_);
            } else if (selected_ == 1 && !manual_url_.empty()) {
                if (is_valid_protocol(manual_url_)) {
                    manual_list_.items.push_back(normalize_url(manual_url_));
                    manual_url_ = "";
                } else {
                    ErrorPopup::show("Validation Error", "Invalid protocol (https/ftp/rsync required)");
                }
            } else if (selected_ == 2) {
                if (YesNoPopup::show("Confirm Clear", "Are you sure you want to clear all added mirrors?")) manual_list_.items.clear();
            } else if (selected_ == 3) {
                if (!manual_list_.items.empty()) {
                    manual_list_.is_focused = true;
                } else {
                    ErrorPopup::show("Notice", "List is empty.");
                }
            } else if (selected_ == 4) {
                // SAVE: Commit to main list
                for (const auto& m : manual_list_.items) {
                    DataStore::instance().mirrors.push_back({m, "Manual", true});
                }
                manual_list_.items.clear();
                section_ = MirrorSection::Main;
                selected_ = 1;
            } else if (selected_ == 5) {
                // DO NOT SAVE: Just discard
                manual_list_.items.clear();
                section_ = MirrorSection::Main;
                selected_ = 1;
            }
            return true;
        }
        return false;
    }

    bool handle_settings(int ch) {
        if (ch == KEY_UP && selected_ > 0) { selected_--; return true; }
        if (ch == KEY_DOWN && selected_ < 5) { selected_++; return true; }
        if (ch == '\n' || ch == KEY_ENTER || ch == ' ') {
            if (selected_ < 4) {
                // For settings, it might be better to just toggle in place as it was, 
                // but since the user approved FormPopup, let's use it for a "Configure All" button 
                // or just apply it to the toggles. 
                // Actually, let's use FormPopup for a more comprehensive configuration.
                std::vector<FormField> fields = {
                    {"Parallel Downloads", "Enable downloading multiple packages at once", DataStore::instance().pacman_cfg.parallel_downloads ? "true" : "false", 5, FieldType::Boolean},
                    {"Max Parallel", "Number of concurrent downloads", std::to_string(DataStore::instance().pacman_cfg.max_parallel), 2, FieldType::Text},
                    {"Color", "Enable colorized pacman output", DataStore::instance().pacman_cfg.color ? "true" : "false", 5, FieldType::Boolean},
                    {"Check Space", "Check for available disk space before installing", DataStore::instance().pacman_cfg.check_space ? "true" : "false", 5, FieldType::Boolean},
                    {"Verbose Lists", "Show more detailed package lists", DataStore::instance().pacman_cfg.verbose_pkg_lists ? "true" : "false", 5, FieldType::Boolean}
                };
                if (FormPopup::show("Pacman Settings", fields)) {
                    DataStore::instance().pacman_cfg.parallel_downloads = (fields[0].value == "true" || fields[0].value == "1" || fields[0].value == "y");
                    try { DataStore::instance().pacman_cfg.max_parallel = std::stoi(fields[1].value); } catch(...) {}
                    DataStore::instance().pacman_cfg.color = (fields[2].value == "true" || fields[2].value == "1" || fields[2].value == "y");
                    DataStore::instance().pacman_cfg.check_space = (fields[3].value == "true" || fields[3].value == "1" || fields[3].value == "y");
                    DataStore::instance().pacman_cfg.verbose_pkg_lists = (fields[4].value == "true" || fields[4].value == "1" || fields[4].value == "y");
                }
            } else if (selected_ == 4 || selected_ == 5) { 
                section_ = MirrorSection::Main; 
                selected_ = 2; 
            }
            return true;
        }
        return false;
    }
};

