#ifndef ULI_RUNTIME_SELECTION_UI_HPP
#define ULI_RUNTIME_SELECTION_UI_HPP

#include <iostream>
#include <string>
#include <vector> // Added based on the provided "Code Edit" block
#include <sstream> // Added based on the provided "Code Edit" block
#include <iomanip>
#include "menu.hpp"
#include "design_ui.hpp"
#include "dialogbox.hpp"

#include "ui_menudesign.hpp"
#include "contents/translations/lang_export.hpp" // This line was present and is kept. The instruction text was confusing but the "Code Edit" block kept it.

namespace uli {
namespace runtime {

class SelectionUI {
public:
    // Helper to format a single menu line aligned properly
    static std::string format_menu_item(const std::string& label, const std::string& value) {
        std::stringstream ss;
        ss << std::left << std::setw(36) << _tr(label);
        ss << DesignUI::RESET; // Stop the background highlight here
        ss << DesignUI::CYAN << "set: " << DesignUI::RESET << value;
        return ss.str();
    }

    // Renders the overarching config menu modeled after archinstall (now interactive)
    static std::string render_main_menu(const MenuState& state, int& last_selected, const std::string& os_distro) {
        std::vector<std::string> options;
        std::vector<std::string> actions;

        auto add_opt = [&](const std::string& a, const std::string& l, const std::string& v) {
            options.push_back(format_menu_item(l, v));
            actions.push_back(a);
        };

        add_opt("LANG", "Installation language", state.language);
        add_opt("KBD", "Keyboard layout", state.keyboard_layout);
        
        std::string mirror_display = state.active_mirrors.empty() ? "None" : (state.active_mirrors.size() == 1 ? state.active_mirrors[0] : std::to_string(state.active_mirrors.size()) + " selected");
        add_opt("MIRROR", "Mirror region", mirror_display);

        add_opt("REPOS", "Optional repositories", state.get_optional_repos_str(os_distro));

        add_opt("LOCALE_LANG", "Locale language", state.locale_language);
        add_opt("LOCALE_ENC", "Locale encoding", state.locale_encoding);
        add_opt("DRIVE", "Drive(s)", state.drive.empty() ? "None" : state.drive);
        add_opt("BOOT", "Bootloader", state.get_bootloader_str());
        add_opt("HOST", "Hostname", state.hostname);
        std::string root_pwd_display = (state.root_password == "None" || state.root_password.empty()) ? "None" : "Set";
        add_opt("ROOT", "Root password", root_pwd_display);
        add_opt("USER", "User account", state.get_users_str());
        add_opt("PROFILE", "Profile", state.profile);
        add_opt("AUDIO", "Audio", state.audio);
        add_opt("KERNEL", "Kernels", "['" + state.kernel + "']");
        add_opt("PKGS", "Additional packages", state.get_packages_str());
        add_opt("NET", "Network configuration", state.network_configuration);
        add_opt("TZ", "Timezone", state.timezone);
        add_opt("NTP", "Automatic time sync (NTP)", (state.ntp_sync ? "True" : "False"));
        
        options.push_back(""); actions.push_back("SPACER");
        
        options.push_back(std::string(DesignUI::DIM) + "> " + _tr("Save configuration") + DesignUI::RESET);
        actions.push_back("SAVE");
        
        if (state.drive.empty()) {
            options.push_back(std::string(DesignUI::DIM) + "> " + _tr("Install (1 config(s) missing)") + DesignUI::RESET);
        } else {
            options.push_back(std::string(DesignUI::GREEN) + "> " + _tr("Install") + DesignUI::RESET);
        }
        actions.push_back("INSTALL");

        options.push_back(std::string(DesignUI::RED) + "> " + _tr("Abort") + DesignUI::RESET);
        actions.push_back("ABORT");

        int sel = UIMenuDesign::show_interactive_list("Universal Linux Installer Engine", options, last_selected);
        if (sel != -1) {
            last_selected = sel;
            return actions[sel];
        }
        return "NONE";
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_SELECTION_UI_HPP
