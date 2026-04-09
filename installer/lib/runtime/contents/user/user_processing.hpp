#ifndef ULI_RUNTIME_USER_PROCESSING_HPP
#define ULI_RUNTIME_USER_PROCESSING_HPP

#include "../../dialogbox.hpp"
#include "../../menu.hpp"
#include "../translations/lang_export.hpp"
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace uli {
namespace runtime {
namespace contents {
namespace user {

class UserProcessing {
public:
  static std::string dynamic_user_banner_func(const MenuState &state,
                                              const std::string &os_distro) {
    if (state.users.empty())
      return "";
    std::stringstream ss;
    ss << "\n";

    bool ext = (os_distro == "Debian" );
    size_t u_w = 8;
    size_t n_w = 9;

    for (const auto &u : state.users) {
      if (u.username.length() > u_w)
        u_w = u.username.length();
      if (ext && u.full_name.length() > n_w)
        n_w = u.full_name.length();
    }

    ss << "  | " << std::left << std::setw(7) << "User ID"
       << " | " << std::setw(u_w) << "Username";
    if (ext)
      ss << " | " << std::setw(n_w) << "Full Name";
    ss << " | " << std::setw(8) << "Password"
       << " | " << std::setw(4) << "Sudo"
       << " | " << std::setw(5) << "Wheel"
       << " | " << std::setw(8) << "Home Dir" << " |\n";

    ss << "  |-" << std::string(7, '-') << "-|-" << std::string(u_w, '-');
    if (ext)
      ss << "-|-" << std::string(n_w, '-');
    ss << "-|-" << std::string(8, '-') << "-|-" << std::string(4, '-') << "-|-"
       << std::string(5, '-') << "-|-" << std::string(8, '-') << "-|\n";

    int uid = 1000;
    for (const auto &u : state.users) {
      ss << "  | " << std::left << std::setw(7) << uid++;
      ss << " | " << std::setw(u_w) << u.username;
      if (ext)
        ss << " | " << std::setw(n_w) << u.full_name;

      std::string pw_mask =
          std::string(std::min((size_t)8, u.password.length()), '*');
      ss << " | " << std::setw(8) << pw_mask;
      ss << " | " << std::setw(4) << (u.is_sudoer ? "Yes" : "No");

      ss << " | " << std::setw(5) << (u.is_wheel ? "Yes" : "No");

      ss << " | " << std::setw(8) << (u.create_home ? "Yes" : "No") << " |\n";
    }

    ss << "\n";
    return ss.str();
  }

  static void run_user_config(MenuState &state, const std::string &os_distro) {
    bool main_loop = true;
    while (main_loop) {
      auto banner_func = [&state, &os_distro]() {
        return dynamic_user_banner_func(state, os_distro);
      };

      std::vector<std::string> opts = {_tr("Add a User"), _tr("Modify a User"),
                                       _tr("Clear Users list"), _tr("Done")};

      int res = DialogBox::ask_selection(
          _tr("User Accounts"), _tr("Manage standard non-root shell users"),
          opts, banner_func);

      if (res == 2) { // Clear
        state.users.clear();
        DialogBox::show_alert(_tr("User Accounts"),
                              _tr("List cleared successfully."), banner_func);
      } else if (res == 3 || res == -1) { // Done
        break;
      } else if (res == 1) { // Modify
        if (state.users.empty()) {
          DialogBox::show_alert(
              _tr("Error"), _tr("No users available to modify."), banner_func);
          continue;
        }

        std::vector<std::string> mod_opts;
        for (size_t i = 0; i < state.users.size(); ++i) {
          mod_opts.push_back(state.users[i].username);
        }

        int u_idx = DialogBox::ask_selection(_tr("Modify User"),
                                             _tr("Select user to edit:"),
                                             mod_opts, banner_func);
        if (u_idx == -1)
          continue;

        auto &u = state.users[u_idx];
        bool mod_loop = true;
        while (mod_loop) {
          std::vector<std::string> props = {
              _tr("Username (Current: ") + u.username + ")",
              _tr("Password (Current: ") +
                  std::string(std::min((size_t)8, u.password.length()), '*') +
                  ")",
              _tr("Full Name (Current: ") + u.full_name + ")",
              _tr("Toggle 'sudo' Elevation Privileges (Current: ") +
                  (u.is_sudoer ? "Yes)" : "No)"),
              _tr("Toggle 'wheel' Elevation Privileges (Current: ") +
                  (u.is_wheel ? "Yes)" : "No)"),
              _tr("Toggle Home Directory Creation (Current: ") +
                  (u.create_home ? "Yes)" : "No)"),
              _tr("Delete Account"),
              _tr("Done modifying")};

          int prop_idx = DialogBox::ask_selection(
              _tr("Edit User: ") + u.username, _tr("Select property to alter:"),
              props, banner_func);
          if (prop_idx == 7 || prop_idx == -1)
            break;

          if (prop_idx == 0) { // Username
            std::string nu = DialogBox::ask_input(
                _tr("Modify Username"), _tr("Enter new username:"), u.username);
            if (!nu.empty())
              u.username = nu;
          } else if (prop_idx == 1) { // Password
            std::string np = DialogBox::ask_password(
                _tr("Modify Password"),
                _tr("Set a new password for ") + u.username + ":");
            if (!np.empty())
              u.password = np;
          } else if (prop_idx == 2) { // Full Name
            u.full_name =
                DialogBox::ask_input(_tr("Modify Full Name"),
                                     _tr("Enter new Full Name:"), u.full_name);
          } else if (prop_idx == 3) { // Sudo Privs
            u.is_sudoer = DialogBox::ask_yes_no(
                _tr("Sudo Privileges"), _tr("Grant 'sudo' group elevation privileges to ") +
                                       u.username + "?");
          } else if (prop_idx == 4) { // Wheel Privs
            u.is_wheel = DialogBox::ask_yes_no(
                _tr("Wheel Privileges"), _tr("Grant 'wheel' group elevation privileges to ") +
                                       u.username + "?");
          } else if (prop_idx == 5) { // Home
            u.create_home = DialogBox::ask_yes_no(
                _tr("Home Directory"),
                _tr("Create an active Home Directory (/home/") + u.username +
                    ")?");
          } else if (prop_idx == 6) { // Delete
            state.users.erase(state.users.begin() + u_idx);
            break;
          }
        }
      } else if (res == 0) { // Add User
        UserConfig new_user;

        new_user.username = DialogBox::ask_input(
            _tr("Username"),
            _tr("Enter a standard unprivileged username (e.g., john):"));
        if (new_user.username.empty())
          continue;

        new_user.password = DialogBox::ask_password(
            _tr("Password"), _tr("Enter a password for ") + new_user.username +
                                 _tr(" (Characters will be masked):"));
        if (new_user.password.empty()) {
          DialogBox::show_alert(_tr("Error"),
                                _tr("A secure password is required to "
                                    "instantiate this user account."),
                                banner_func);
          continue;
        }

        // Gather extended properties common in Debian
        if (os_distro == "Debian" ) {
          if (DialogBox::ask_yes_no(
                  _tr("Extended Information"),
                  _tr("Would you like to assign \"Full Name\", \"Room "
                      "Number\", and Phone metadata for this account?"))) {
            new_user.full_name = DialogBox::ask_input(
                _tr("Full Name"), _tr("Enter the Full Name binding for ") +
                                      new_user.username + _tr(":"));
            new_user.room_number = DialogBox::ask_input(
                _tr("Room Number"), _tr("Room Number (optional):"));
            new_user.work_phone = DialogBox::ask_input(
                _tr("Work Phone"), _tr("Office Phone (optional):"));
            new_user.home_phone = DialogBox::ask_input(
                _tr("Home Phone"), _tr("Home Phone (optional):"));
            new_user.other = DialogBox::ask_input(_tr("Other Info"),
                                                  _tr("Other (optional):"));
          }
        }

        // Evaluate privileges securely
        new_user.is_sudoer = DialogBox::ask_yes_no(
            _tr("Sudo Privileges"),
            _tr("Should ") + new_user.username +
                _tr(" be added to the 'sudo' elevation group?"));

        new_user.is_wheel = DialogBox::ask_yes_no(
            _tr("Wheel Privileges"),
            _tr("Should ") + new_user.username +
                _tr(" be added to the 'wheel' elevation group?"));

        state.users.push_back(new_user);
        DialogBox::show_alert(_tr("Success"),
                              _tr("User account ") + new_user.username +
                                  _tr(" added temporarily."),
                              banner_func);
      }
    }
  }
};

} // namespace user
} // namespace contents
} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_USER_PROCESSING_HPP
