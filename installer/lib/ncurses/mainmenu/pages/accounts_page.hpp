#pragma once
// accounts_page.hpp - Root and User Accounts configuration
#include "page.hpp"
#include "../../ncurseslib.hpp"
#include <vector>
#include <string>

struct UserAccount {
    std::string username;
    std::string password;   // stored for sim only
    bool        in_wheel;
};

class AccountsPage : public Page {
    std::string root_password_;
    std::vector<UserAccount> users_;
    int focus_ = 0;       // 0=root section, 1=user list, 2=add user
    int user_selected_ = 0;

public:
    AccountsPage() {}

    std::string title() const override { return "Root and User Accounts"; }

    void render(WINDOW* win) override {
        int h, w;
        getmaxyx(win, h, w);

        // ── Root Password Section ──
        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 1, 2, "Root Account");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        mvwprintw(win, 3, 4, "Root Password: ");
        if (root_password_.empty()) {
            wattron(win, COLOR_PAIR(CP_CHECKBOX_OFF));
            mvwprintw(win, 3, 19, "[NOT SET]");
            wattroff(win, COLOR_PAIR(CP_CHECKBOX_OFF));
        } else {
            wattron(win, COLOR_PAIR(CP_CHECKBOX_ON));
            mvwprintw(win, 3, 19, "[SET - %d chars]", (int)root_password_.size());
            wattroff(win, COLOR_PAIR(CP_CHECKBOX_ON));
        }

        if (focus_ == 0) {
            wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
            mvwprintw(win, 4, 4, " [Press ENTER to set root password] ");
            wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
        } else {
            mvwprintw(win, 4, 4, "  Press ENTER to set root password  ");
        }

        NcursesLib::draw_hline(win, 6, 1, w - 2);

        // ── User Accounts Section ──
        wattron(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);
        mvwprintw(win, 7, 2, "User Accounts");
        wattroff(win, COLOR_PAIR(CP_SECTION_TITLE) | A_BOLD);

        if (users_.empty()) {
            mvwprintw(win, 9, 4, "No users created yet.");
        } else {
            wattron(win, COLOR_PAIR(CP_TABLE_HEADER) | A_BOLD | A_UNDERLINE);
            mvwprintw(win, 9, 4, "%-20s %-10s %-8s", "Username", "Password", "Wheel");
            wattroff(win, COLOR_PAIR(CP_TABLE_HEADER) | A_BOLD | A_UNDERLINE);

            for (int i = 0; i < (int)users_.size() && i < h - 14; i++) {
                int y = 10 + i;
                if (focus_ == 1 && i == user_selected_) {
                    wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                    mvwhline(win, y, 3, ' ', w - 6);
                } else {
                    wattron(win, COLOR_PAIR(CP_NORMAL));
                }
                mvwprintw(win, y, 4, "%-20s %-10s %-8s",
                    users_[i].username.c_str(),
                    "********",
                    users_[i].in_wheel ? "Yes" : "No");
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
                wattroff(win, COLOR_PAIR(CP_NORMAL));
            }
        }

        // Add/Delete user actions
        int act_y = std::max(11, 10 + (int)users_.size() + 1);
        if (act_y < h - 3) {
            NcursesLib::draw_hline(win, act_y, 1, w - 2);
            if (focus_ == 2) {
                wattron(win, COLOR_PAIR(CP_HIGHLIGHT));
                mvwprintw(win, act_y + 1, 4, " [Add New User] ");
                wattroff(win, COLOR_PAIR(CP_HIGHLIGHT));
            } else {
                mvwprintw(win, act_y + 1, 4, "  Add New User  ");
            }

            if (!users_.empty()) {
                mvwprintw(win, act_y + 2, 4, "  DEL key to remove selected user");
            }
        }
    }

    bool handle_input(WINDOW* win, int ch) override {
        if (ch == '\t') {
            focus_ = (focus_ + 1) % 3;
            if (focus_ == 1 && users_.empty()) focus_ = 2;
            return true;
        }

        if (focus_ == 0) {
            if (ch == '\n' || ch == KEY_ENTER) {
                std::string pw = NcursesLib::masked_input(win, 4, 5, 30, 64);
                if (!pw.empty()) root_password_ = pw;
                return true;
            }
        } else if (focus_ == 1) {
            if (ch == KEY_UP && user_selected_ > 0) { user_selected_--; return true; }
            if (ch == KEY_DOWN && user_selected_ < (int)users_.size() - 1) { user_selected_++; return true; }
            if ((ch == KEY_DC || ch == 'd') && !users_.empty()) {
                users_.erase(users_.begin() + user_selected_);
                if (user_selected_ >= (int)users_.size() && user_selected_ > 0)
                    user_selected_--;
                if (users_.empty()) focus_ = 2;
                return true;
            }
        } else if (focus_ == 2) {
            if (ch == '\n' || ch == KEY_ENTER) {
                // Simulate adding a user
                std::string uname = NcursesLib::text_input(win, 7, 18, 20, 32);
                if (!uname.empty()) {
                    std::string pw = NcursesLib::masked_input(win, 8, 18, 20, 64);
                    if (!pw.empty()) {
                        users_.push_back({uname, pw, true});
                    }
                }
                return true;
            }
        }
        return false;
    }
};
