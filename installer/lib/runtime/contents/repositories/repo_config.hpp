#ifndef ULI_RUNTIME_CONTENTS_REPOSITORIES_REPO_CONFIG_HPP
#define ULI_RUNTIME_CONTENTS_REPOSITORIES_REPO_CONFIG_HPP

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "../../menu.hpp"
#include "../../dialogbox.hpp"

namespace uli {
namespace runtime {
namespace contents {
namespace repositories {

class RepoConfig {
public:
    static void run_interactive(MenuState& state, const std::string& os_distro) {
        if (os_distro == "Alpine Linux") {
            run_alpine_repos(state);
        } else if (os_distro == "Arch Linux") {
            run_arch_repos(state);
        } else if (os_distro == "Debian" || os_distro == "Ubuntu") {
            run_debian_repos(state, os_distro);
        }
    }

private:
    // ─────────────────────────────────────────────────
    // Alpine Linux: Toggle main/community/testing + custom
    // ─────────────────────────────────────────────────
    static void run_alpine_repos(MenuState& state) {
        bool in_menu = true;
        while(in_menu) {
            std::vector<std::string> opts;
            opts.push_back(state.alpine_repo_main ? "[x] main (core)" : "[ ] main (core)");
            opts.push_back(state.alpine_repo_community ? "[x] community" : "[ ] community");
            opts.push_back(state.alpine_repo_testing ? "[x] testing" : "[ ] testing");
            int custom_c = state.optional_repos.size();
            opts.push_back("Custom Repositories (" + std::to_string(custom_c) + " injected)");
            opts.push_back("Done / Back");

            int res = DialogBox::ask_selection("Alpine Linux Repositories", "Toggle official sources or inject custom APK URIs.", opts);
            if (res == 0) {
                state.alpine_repo_main = !state.alpine_repo_main;
            } else if (res == 1) {
                state.alpine_repo_community = !state.alpine_repo_community;
            } else if (res == 2) {
                state.alpine_repo_testing = !state.alpine_repo_testing;
            } else if (res == 3) {
                std::string custom = DialogBox::ask_input("Custom Repository", "Enter full repository URI (e.g., http://dl-cdn.alpinelinux.org/alpine/v3.18/main):");
                if (!custom.empty()) state.optional_repos.push_back("APK|" + custom);
            } else {
                if (!state.alpine_repo_main) {
                    DialogBox::show_alert("Warning: Missing Sources", "You have disabled 'main'. Core packages will not be obtainable unless a custom server mirrors them.");
                }
                in_menu = false;
            }
        }
    }


    // ─────────────────────────────────────────────────
    // Arch Linux: Toggle core/extra/multilib + custom
    // ─────────────────────────────────────────────────
    static void run_arch_repos(MenuState& state) {
        bool in_menu = true;
        while(in_menu) {
            std::vector<std::string> opts;
            opts.push_back(state.arch_repo_core ? "[x] core" : "[ ] core");
            opts.push_back(state.arch_repo_extra ? "[x] extra" : "[ ] extra");
            opts.push_back(state.arch_repo_multilib ? "[x] multilib (32-bit compatibility)" : "[ ] multilib (32-bit compatibility)");
            int custom_c = state.optional_repos.size();
            opts.push_back("Custom Servers (" + std::to_string(custom_c) + " injected)");
            opts.push_back("Done / Back");

            int res = DialogBox::ask_selection("Arch Linux Repositories", "Toggle official sources or inject custom Mirrors.", opts);
            if (res == 0) state.arch_repo_core = !state.arch_repo_core;
            else if (res == 1) state.arch_repo_extra = !state.arch_repo_extra;
            else if (res == 2) state.arch_repo_multilib = !state.arch_repo_multilib;
            else if (res == 3) {
                std::string custom = DialogBox::ask_input("Custom Repository", 
                    "Enter repository segment (e.g., [custom]\nServer = http...)\n\n"
                    "Note: If adding multiple custom mirrors, you may prefer:\n"
                    "Include = /etc/pacman.d/customlist\n"
                    "to avoid conflicts with existing mirrorlistings.");
                if (!custom.empty()) state.optional_repos.push_back(custom);
            }
            else {
                if (!state.arch_repo_core || !state.arch_repo_extra) {
                    DialogBox::show_alert("Warning: Official Repos Disabled", 
                        "You have disabled 'core' and/or 'extra'.\n\n"
                        "You can no longer get packages or updates from the official Arch repo sources unless you have custom mirrors configured.");
                }
                in_menu = false;
            }
        }
    }

    // ─────────────────────────────────────────────────
    // Debian / Ubuntu: Version-aware repo format
    // ─────────────────────────────────────────────────
    static void run_debian_repos(MenuState& state, const std::string& os_distro) {
        int ver = state.debian_version;

        // Determine which format modes to offer
        // Ubuntu: always classic only (Ubuntu hasn't adopted deb822 as default)
        // Debian <= 12: classic only
        // Debian >= 13: deb822 only
        // Debian unknown (0): show both as fallback
        bool show_classic = false;
        bool show_deb822 = false;

        if (os_distro == "Ubuntu") {
            show_classic = true;
            show_deb822 = false;
        } else {
            // Debian
            if (ver == 0) {
                // Could not determine version — offer both
                show_classic = true;
                show_deb822 = true;
            } else if (ver <= 12) {
                show_classic = true;
                show_deb822 = false;
            } else {
                // >= 13
                show_classic = false;
                show_deb822 = true;
            }
        }

        // Build version hint for title
        std::string title = os_distro + " Repositories";
        if (ver > 0 && os_distro == "Debian") {
            title += " (Debian " + std::to_string(ver) + " detected)";
        } else if (ver == 0 && os_distro == "Debian") {
            title += " (Version unknown - showing all formats)";
        }

        bool in_menu = true;
        while (in_menu) {
            std::vector<std::string> opts;
            
            if (show_classic) {
                opts.push_back("Add Custom single-line Repository (sources.list)");
            }
            if (show_deb822) {
                opts.push_back("Add Custom deb822 Repository (sources.list.d/)");
            }
            if (!state.optional_repos.empty()) {
                opts.push_back("View/Clear custom injections (" + std::to_string(state.optional_repos.size()) + " added)");
            }
            opts.push_back("Back");

            int res = DialogBox::ask_selection(title, "Current Custom Additions: " + std::to_string(state.optional_repos.size()), opts);
            
            if (res < 0 || (size_t)res >= opts.size()) {
                in_menu = false;
                continue;
            }

            std::string chosen = opts[res];

            if (chosen.find("single-line") != std::string::npos) {
                prompt_classic_repo(state);
            } else if (chosen.find("deb822") != std::string::npos) {
                prompt_deb822_repo(state);
            } else if (chosen.find("View/Clear") != std::string::npos) {
                state.optional_repos.clear();
                DialogBox::show_alert("Cleared", "All custom repository injections have been removed.");
            } else if (chosen == "Back") {
                in_menu = false;
            }
        }
    }

    // ── Classic sources.list single-line prompt ──
    static void prompt_classic_repo(MenuState& state) {
        bool in_menu = true;
        while (in_menu) {
            std::vector<std::string> opts;
            std::vector<std::string> sources;
            std::string header_msg = "Host /etc/apt/sources.list status: ";

            std::ifstream file("/etc/apt/sources.list");
            if (!file.is_open()) {
                header_msg += "NON-EXISTENT!";
            } else {
                std::string temp;
                while(std::getline(file, temp)) {
                    // skip comments and empty lines
                    if (temp.empty() || temp[0] == '#') continue;
                    sources.push_back(temp);
                }
                if (sources.empty()) {
                    header_msg += "BLANK/EMPTY!";
                } else {
                    header_msg += "Found " + std::to_string(sources.size()) + " active entries.";
                }
            }

            for (size_t i = 0; i < sources.size(); ++i) {
                opts.push_back("Import: " + sources[i]);
            }
            opts.push_back("Manual Entry (Type a new repository)");
            opts.push_back("Back");

            int res = DialogBox::ask_selection("Classic Repository", header_msg, opts);
            if (res < 0 || (size_t)res >= opts.size()) {
                in_menu = false;
                continue;
            }

            std::string chosen = opts[res];
            if (chosen == "Back") {
                in_menu = false;
            } else if (chosen.find("Manual Entry") != std::string::npos) {
                std::string line = DialogBox::ask_input(
                    "Classic Repository",
                    "Enter APT source line:\ne.g. deb http://deb.debian.org/debian bookworm main"
                );
                if (!line.empty()) {
                    state.optional_repos.push_back("CLASSIC|" + line);
                    DialogBox::show_alert("Added", "Custom repository added.");
                }
            } else if (chosen.find("Import: ") == 0) {
                std::string imported = chosen.substr(8); // remove "Import: "
                state.optional_repos.push_back("CLASSIC|" + imported);
                DialogBox::show_alert("Imported", "Successfully imported:\n" + imported);
            }
        }
    }

    // ── deb822 guided builder prompt ──
    static void prompt_deb822_repo(MenuState& state) {
        std::string uris = DialogBox::ask_input("deb822 Builder", "Enter URIs (e.g. http://deb.debian.org/debian/):");
        if (uris.empty()) return;
        std::string suites = DialogBox::ask_input("deb822 Builder", "Enter Suites (e.g. trixie trixie-updates):");
        std::string comps = DialogBox::ask_input("deb822 Builder", "Enter Components (e.g. main contrib non-free-firmware):");
        std::string signedby = DialogBox::ask_input("deb822 Builder", "Enter Signed-By path (Optional, leave blank to skip):");

        std::string block = "DEB822|\nTypes: deb\nURIs: " + uris + "\nSuites: " + suites + "\nComponents: " + comps;
        if (!signedby.empty()) block += "\nSigned-By: " + signedby;
        
        state.optional_repos.push_back(block);
    }
};

} // namespace repositories
} // namespace contents
} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_CONTENTS_REPOSITORIES_REPO_CONFIG_HPP
