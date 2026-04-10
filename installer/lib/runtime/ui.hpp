#ifndef ULI_RUNTIME_UI_HPP
#define ULI_RUNTIME_UI_HPP

#include "../config/config_loader.hpp"
#include "../config/persistence_manager.hpp"
#include "process_supervisor.hpp"

#include <iostream>
#include <string>
#include "design_ui.hpp"
#include "dialogbox.hpp"
#include "warn.hpp"
#include "dependency_check.hpp"
#include "handler.hpp"
#include "post_installer.hpp"
#include "network_metrics.hpp"

#include "contents/translations/lang_export.hpp"
#include "contents/translations/english.hpp"
#include "contents/translations/filipino.hpp"
#include "contents/translations/spanish.hpp"
#include "contents/translations/french.hpp"
#include "contents/translations/german.hpp"
#include "contents/translations/italian.hpp"
#include "contents/kb_layout/kblayout.hpp"
#include "contents/locale/locale_lang.hpp"
#include "contents/locale/locale_encode.hpp"
#include "selection_ui.hpp"
#include "ui_multiselect.hpp"
#include "../config/config_exporter.hpp"
#include "../localegen/localegen.hpp"
#include "../package_mgr/dpkg_apt/mirrorlist_debian.hpp"
#include "../package_mgr/apk/mirrorlist.hpp"
#include "../efi_check/efi_check.hpp"
#include "contents/user/user_processing.hpp"
#include "contents/profiles/profiles.cpp"
#include "../network_mgr/connect.hpp"
#include "../timedate/ui_tz_select.hpp"
#include "contents/repositories/repo_config.hpp"
#include "contents/repositories/repo_validator.hpp"
namespace uli {
namespace runtime {

class UIManager {
public:
    static void start_ui(const std::string& os_distro, int debian_version = 0, const MenuState& initial_state = MenuState()) {
        MenuState state = initial_state;
        if (state.debian_version == 0) state.debian_version = debian_version;
        
        if (os_distro == "Debian") state.bootloader = "grub";
        else if (os_distro == "Alpine Linux") state.bootloader = "syslinux";
        
        // Initial underlying checks
        if (uli::runtime::config::PersistenceManager::has_recovery()) {
            if (DialogBox::ask_yes_no(_tr("Recovery Available"), _tr("An interrupted installation was detected. Would you like to resume with your previous settings?"))) {
                uli::runtime::config::PersistenceManager::load_recovery(state);
            }
        }

        if (NetworkMetrics::run_diagnostics()) {
            state.network_configuration = "Connected";
        } else {
            DialogBox::show_alert(_tr("No Internet Connection"), _tr("Warning: Running the installation requires an active internet connection to download packages.\n\nYou can configure the network later in the installer options."));
            state.network_configuration = "Not configured";
        }

        int selected_index = 0;
        while (true) {
            std::string action = SelectionUI::render_main_menu(state, selected_index, os_distro);
            if (action != "NONE") {
                // Keep cursor memory internally managed by reference
            }

            if (action == "ABORT") {
                Warn::print_warning("Installation Aborted.");
                return;
            } else if (action == "INSTALL") {
                std::string missing = "";
                if (state.drive.empty() || state.drive == "None") missing += "Drive, ";
                if (state.root_password == "None" || state.root_password.empty()) missing += "Root Password, ";
                if (state.users.empty()) missing += "User Account, ";
                if (state.profile == "None" || state.profile.empty()) missing += "Profile, ";
                if (state.network_configuration == "Not configured" || state.network_configuration.empty()) missing += "Network config, ";
                
                if (!missing.empty()) {
                     missing.pop_back(); missing.pop_back(); // Remove trailing comma
                     DialogBox::show_alert(_tr("Installation Blocked"), _tr("Missing required configs: ") + missing);
                     continue;
                }
                
                if (DialogBox::ask_yes_no(_tr("Confirm Installation"), _tr("Begin formatting and installing to ") + state.drive + _tr("? This action is destructive!"))) {
                    break;
                }
            } else if (action == "SAVE") {
                std::string path = "uli_config_dump.yaml";
                if (uli::runtime::config::ConfigExporter::save_menu_state_to_yaml(state, path)) {
                    DialogBox::show_alert(_tr("Success"), _tr("Configuration saved successfully to: ") + path);
                }
            } else if (action == "LANG") { // Installation Language
                std::vector<std::string> langs = {"English", "Filipino", "Spanish", "French", "German", "Italian"};
                int res = DialogBox::ask_selection(_tr("Language"), _tr("Select display language"), langs);
                if (res != -1) {
                    state.language = langs[res];
                    if (langs[res] == "Filipino") {
                       uli::runtime::contents::translations::active_dict = uli::runtime::contents::translations::get_filipino_dict();
                    } else if (langs[res] == "Spanish") {
                       uli::runtime::contents::translations::active_dict = uli::runtime::contents::translations::get_spanish_dict();
                    } else if (langs[res] == "French") {
                       uli::runtime::contents::translations::active_dict = uli::runtime::contents::translations::get_french_dict();
                    } else if (langs[res] == "German") {
                       uli::runtime::contents::translations::active_dict = uli::runtime::contents::translations::get_german_dict();
                    } else if (langs[res] == "Italian") {
                       uli::runtime::contents::translations::active_dict = uli::runtime::contents::translations::get_italian_dict();
                    } else {
                       uli::runtime::contents::translations::active_dict = uli::runtime::contents::translations::get_english_dict();
                    }
                }
            } else if (action == "KBD") { // Keyboard Layout
                std::vector<std::string> layouts = uli::runtime::contents::kb_layout::KBLayout::get_supported_layouts();
                int res = DialogBox::ask_selection(_tr("Keyboard layout"), _tr("Select standard keyboard ISO layout"), layouts);
                if (res != -1) state.keyboard_layout = layouts[res];
            } else if (action == "MIRROR") { // Mirror Region
                if (os_distro == "Arch Linux") {
                    std::vector<std::string> opts = {_tr("Auto-Optimize (Reflector)"), _tr("Custom Mirror URL (Manual Entry)")};
                    int res = DialogBox::ask_selection(_tr("Mirror Region"), _tr("Select Arch Linux mirror behavior"), opts);
                    if (res == 0) {
                        state.active_mirrors = {"Auto-Optimize (Reflector)"};
                    } else if (res == 1) {
                        std::string url = DialogBox::ask_input(_tr("Mirror Settings"), _tr("Enter full mirror URL. Ref: https://archlinux.org/mirrors/"));
                        if (!url.empty()) state.active_mirrors = {url};
                    }
                } else if (os_distro == "Debian" || os_distro == "Alpine Linux") {
                    std::vector<std::string> initial_opts = {_tr("Use Default Mirrors Only"), _tr("Select custom mirrors...")};
                    int initial_res = DialogBox::ask_selection(_tr("Mirror Region"), _tr("Do you want to use the default repositories or select specific mirrors?"), initial_opts);
                    
                    if (initial_res == 0) {
                        state.active_mirrors = {"Default"};
                    } else if (initial_res == 1) {
                        std::vector<std::pair<std::string, std::string>> mirrors;
                        std::string ref_link;
                        if (os_distro == "Debian") {
                            mirrors = uli::package_mgr::dpkg_apt::MirrorlistDebian::get_debian_mirrors();
                            ref_link = "Ref: https://www.debian.org/mirror/list";
                        } else {
                            mirrors = uli::package_mgr::apk::Mirrorlist::get_alpine_mirrors();
                            ref_link = "Ref: https://mirrors.alpinelinux.org/";
                        }
                        
                        std::vector<std::string> display_list;
                        std::unordered_map<std::string, std::string> mirror_map;
                        
                        display_list.push_back(_tr("Custom Mirror URL (Manual Entry)"));
                        
                        for (const auto& m : mirrors) {
                            std::string entry = m.first + " (" + m.second + ")";
                            display_list.push_back(entry);
                            mirror_map[entry] = m.second;
                        }
                        
                        std::vector<std::string> pre_selected;
                        for (const auto& url : state.active_mirrors) {
                            if (url == "Default") continue;
                            bool found = false;
                            for (const auto& kv : mirror_map) {
                                if (kv.second == url) {
                                    pre_selected.push_back(kv.first);
                                    found = true;
                                    break;
                                }
                            }
                            if (!found && !url.empty()) {
                                std::string custom_entry = "Custom: " + url;
                                display_list.push_back(custom_entry);
                                pre_selected.push_back(custom_entry);
                                mirror_map[custom_entry] = url;
                            }
                        }
                        
                        std::vector<std::string> results = UIMultiSelectDesign::show_multi_selection_list(
                            "Mirror Region", 
                            display_list, 
                            pre_selected, 
                            nullptr, 
                            ref_link
                        );
                        
                        std::vector<std::string> final_mirrors;
                        for (const auto& res : results) {
                            if (res == _tr("Custom Mirror URL (Manual Entry)")) {
                                std::string url = DialogBox::ask_input(_tr("Mirror Settings"), _tr("Enter full explicit mirror URL:"));
                                if (!url.empty()) final_mirrors.push_back(url);
                            } else {
                                final_mirrors.push_back(mirror_map[res]);
                            }
                        }
                        
                        if (!final_mirrors.empty()) {
                            state.active_mirrors = final_mirrors;
                        }
                    }
                } else {
                     DialogBox::show_alert(_tr("Mirror Settings"), _tr("Mirror selection is not natively supported for this distribution yet."));
                }
            } else if (action == "REPOS") { // Optional Repositories Hook
                uli::runtime::contents::repositories::RepoConfig::run_interactive(state, os_distro);
            } else if (action == "LOCALE_LANG") { // Locale Language
                std::vector<std::string> locales = uli::runtime::contents::locale::LocaleLang::get_languages();
                int res = DialogBox::ask_selection(_tr("Locale language"), _tr("Select locale target bindings"), locales);
                if (res != -1) state.locale_language = locales[res];
            } else if (action == "LOCALE_ENC") { // Locale Encoding
                std::vector<std::string> encodes = uli::runtime::contents::locale::LocaleEncode::get_encodings();
                int res = DialogBox::ask_selection(_tr("Locale encoding"), _tr("Select standard locale encoding"), encodes);
                if (res != -1) state.locale_encoding = encodes[res];
            } else if (action == "DRIVE") { // Drive Configuration
                std::string target = UIHandler::handle_disk_configuration(state);
                if (!target.empty()) state.drive = target;
            } else if (action == "BOOT") { // Bootloader
                bool is_efi = uli::efi_check::EFICheck::is_efi_system();
                if (os_distro == "Arch Linux") {
                    std::string sysd_label = "systemd-boot" + std::string(is_efi ? " (RECOMMENDED FOR YOUR SYSTEM)" : "");
                    std::string grub_label = "grub" + std::string(!is_efi ? " (RECOMMENDED FOR YOUR SYSTEM)" : "");
                    std::vector<std::string> opts = {sysd_label, grub_label};
                    
                    int res = DialogBox::ask_selection(_tr("Bootloader"), _tr("Select target bootloader"), opts);
                    if (res != -1) {
                        state.bootloader = (res == 0) ? "systemd-boot" : "grub";
                        
                        if (state.bootloader == "grub") {
                            std::vector<std::string> tgts = {
                                "x86_64-efi (Requires: grub, efibootmgr)", 
                                "i386-pc (Legacy BIOS) (Requires: grub)"
                            };
                            int tres = DialogBox::ask_selection(_tr("Target Firmware"), _tr("Select bootloader target architecture"), tgts);
                            if (tres == 0) state.bootloader_target = "x86_64-efi";
                            else if (tres == 1) state.bootloader_target = "i386-pc";
                        } else {
                            // systemd-boot only supports UEFI
                            state.bootloader_target = "x86_64-efi";
                        }
                        
                        std::string id = DialogBox::ask_input(_tr("Bootloader ID"), _tr("Enter bootloader ID/description (default: Arch Linux):"));
                        if (!id.empty()) state.bootloader_id = id;
                        else state.bootloader_id = "Arch Linux";
                        
                        std::string efi = DialogBox::ask_input(_tr("EFI Directory"), _tr("Where is the EFI System Partition mounted? (default: /boot)"));
                        if (!efi.empty()) state.efi_directory = efi;
                        else state.efi_directory = "/boot";
                    }
                } else if (os_distro == "Debian") {
                    DialogBox::show_alert(_tr("Bootloader"), _tr("Debian natively enforces 'grub' as the default bootloader. Manual selection is bypassed."));
                    state.bootloader = "grub";
                } else if (os_distro == "Alpine Linux") {
                    std::string sysd_label = "syslinux" + std::string(!is_efi ? " (RECOMMENDED FOR YOUR SYSTEM)" : "");
                    std::string grub_label = "grub" + std::string(is_efi ? " (RECOMMENDED FOR YOUR SYSTEM)" : "");
                    std::vector<std::string> opts = {sysd_label, grub_label};
                    
                    int res = DialogBox::ask_selection(_tr("Bootloader"), _tr("Select target bootloader"), opts);
                    if (res != -1) {
                        state.bootloader = (res == 0) ? "syslinux" : "grub";
                        
                        std::vector<std::string> tgts;
                        if (state.bootloader == "grub") {
                            tgts = {
                                "x86_64-efi (Requires: grub-efi, efibootmgr)", 
                                "i386-pc (Legacy BIOS) (Requires: grub-bios)"
                            };
                        } else {
                            tgts = {
                                "x86_64-efi (Requires: syslinux, efibootmgr)", 
                                "i386-pc (Legacy BIOS) (Requires: syslinux)"
                            };
                        }
                        int tres = DialogBox::ask_selection(_tr("Target Firmware"), _tr("Select bootloader target architecture"), tgts);
                        if (tres == 0) state.bootloader_target = "x86_64-efi";
                        else if (tres == 1) state.bootloader_target = "i386-pc";
                        
                        std::string id = DialogBox::ask_input(_tr("Bootloader ID"), _tr("Enter bootloader ID/description (default: Alpine Linux):"));
                        if (!id.empty()) state.bootloader_id = id;
                        else state.bootloader_id = "Alpine Linux";
                        
                        std::string efi = DialogBox::ask_input(_tr("EFI Directory"), _tr("Where is the EFI System Partition mounted? (default: /boot)"));
                        if (!efi.empty()) state.efi_directory = efi;
                        else state.efi_directory = "/boot";
                    }
                } else {
                    DialogBox::show_alert(_tr("Bootloader"), _tr("Bootloader selection is not supported for this distribution."));
                }
            } else if (action == "HOST") { // Hostname
                std::string host = DialogBox::ask_input(_tr("Hostname"), _tr("Set new hostname"));
                if (!host.empty()) state.hostname = host;
            } else if (action == "ROOT") { // Root Password
                std::string pwd = DialogBox::ask_password(_tr("Root Password"), _tr("Set root password (stored securely in state)"));
                if (!pwd.empty()) state.root_password = pwd;
            } else if (action == "USER") { // User Account
                uli::runtime::contents::user::UserProcessing::run_user_config(state, os_distro);
            } else if (action == "PROFILE") { // Profile
                std::vector<std::string> opts = {"Desktop", "Minimal", "Server", "None"};
                int res = DialogBox::ask_selection(_tr("Profile"), _tr("Select standard computing architecture experience"), opts);
                if (res != -1) {
                    state.profile = opts[res];
                    if (state.profile != "None") {
                        uli::runtime::contents::profiles::ProfileManager::apply_profile(state, os_distro);
                        DialogBox::show_alert(_tr("Profile Applied"), _tr("Dependencies for ") + state.profile + _tr(" have been mapped to the package list."));
                    } else {
                        state.additional_packages.clear(); // For simplicity, wiping previous auto-applied packages
                    }
                }
            } else if (action == "AUDIO") { // Audio
                std::vector<std::string> opts = {"pipewire", "pulseaudio (legacy)", "None"};
                int res = DialogBox::ask_selection(_tr("Audio"), _tr("Select audio server"), opts);
                if (res != -1) {
                    state.audio = opts[res];
                }
            } else if (action == "KERNEL") { // Kernels
                if (os_distro == "Arch Linux") {
                    std::vector<std::string> opts = {"linux", "linux-zen", "linux-hardened", "linux-lts"};
                    int res = DialogBox::ask_selection(_tr("Kernel Selection"), _tr("Select standard Linux kernel package:"), opts);
                    if (res != -1) {
                        std::string old_kernel = state.kernel;
                        std::string new_kernel = opts[res];
                        state.kernel = new_kernel;
                        
                        std::string old_headers = old_kernel + "-headers";
                        std::string new_headers = new_kernel + "-headers";
                        for (auto& pkg : state.additional_packages) {
                            if (pkg == old_headers) {
                                pkg = new_headers;
                            }
                        }
                    }
                } else {
                    DialogBox::show_alert(_tr("Kernel Selection"), _tr("Kernel selection is locked to the standard 'linux' package for this distribution."));
                    state.kernel = "linux";
                }
            } else if (action == "PKGS") { // Additional packages
                state.additional_packages = UIHandler::handle_package_search(os_distro, state.additional_packages, state);
            } else if (action == "NET") { // Network configuration
                // Show current network status (ethernet + wireless hint)
                std::string status_report = uli::network_mgr::NetworkConnectHelper::build_network_status_report(state.connected_ssid);
                uli::runtime::DialogBox::show_alert("Network Status", status_report);

                std::vector<std::string> net_opts;
                if (uli::network_mgr::NetworkConnectHelper::has_any_ethernet()) {
                    net_opts.push_back("Stay with wired connectivity");
                }
                net_opts.push_back("Configure Wireless");
                net_opts.push_back("Back");

                int net_sel = uli::runtime::DialogBox::ask_selection("Network Configuration", "Choose an action:", net_opts);
                
                std::string choice = (net_sel >= 0 && (size_t)net_sel < net_opts.size()) ? net_opts[net_sel] : "";

                if (choice == "Stay with wired connectivity") {
                    state.network_configuration = "Wired";
                    state.connected_ssid = "";
                } else if (choice == "Configure Wireless") {
                    std::string ssid_out;
                    if (uli::network_mgr::NetworkConnectHelper::ui_setup_wireless_connection(ssid_out)) {
                        state.connected_ssid = ssid_out;
                        state.network_configuration = "Wireless (SSID: " + ssid_out + ")";
                    }
                }
            } else if (action == "TZ") { // Timezone
                uli::timedate::UITzSetup::run_setup_flow(state);
            } else if (action == "NTP") { // Automatic time sync (NTP)
                uli::timedate::UITzSetup::run_ntp_setup(state);
            }
            // Other indices are purposefully ignored/unimplemented for now while scaffolding
        }

        execute_install(os_distro, state);
    }

    // Static helper to execute the final installation sequence
    static void execute_install(const std::string& os_distro, MenuState& state) {
        // 0. Preliminary Dependency Check
        auto dep_result = uli::runtime::DependencyChecker::check_essentials(os_distro);
        if (!dep_result.success) {
            std::string msg = "Critical system tools are missing for " + os_distro + " installation:\n\n" + 
                               dep_result.missing_tools + "\n\n" +
                               "Please install these dependencies (e.g., 'apt install gdisk' or 'pacman -S gdisk') and restart.";
            DialogBox::show_alert("System Readiness Failure", msg);
            return;
        }

        // Cache state for recovery
        uli::runtime::config::PersistenceManager::save_recovery(state);

        while (true) { // Hard Restart Loop
            DesignUI::clear_screen();
            Warn::print_info("Installation sequence initiated for " + os_distro);
            Warn::print_info("Target Environment: " + state.drive);
            Warn::print_info("Controls: Ctrl+U (Soft), Ctrl+O (Hold), Ctrl+R (Hard), Ctrl+H (Help Guide)");
            
            // 1. Run the deferred format destruction layout logic
            if (!uli::runtime::UIHandler::execute_deferred_partitions(state)) {
                Warn::print_error("Halting Installation: Disk Format failed.");
                uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
                return;
            }

            // 2. Mount partitions and activate swap (Arch Specific)
            if (!uli::runtime::UIHandler::mount_all_partitions(state, os_distro)) {
                Warn::print_error("Halting Installation: Mounting failed.");
                uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
                return;
            }

            bool retry_packages = true;
            while (retry_packages) { // Soft Restart Loop
                retry_packages = false; // Default to finish

                // 3. Refine package list and build final command
                auto final_pkgs = uli::runtime::UIHandler::refine_package_list(os_distro, state);
                
                std::unique_ptr<uli::package_mgr::PackageManagerInterface> pm;
                if (os_distro == "Arch Linux") pm = std::make_unique<uli::package_mgr::alps::AlpsManager>();
                else if (os_distro == "Alpine Linux") pm = std::make_unique<uli::package_mgr::apk::ApkManager>();
                else pm = std::make_unique<uli::package_mgr::DpkgAptManager>();
                
                // Enable Bootstrap mode for installer (targets /mnt)
                setenv("ULI_BOOTSTRAP", "1", 1);
                if (os_distro == "Debian") {
                    std::string suite = (state.debian_version == 12) ? "bookworm" : "trixie";
                    setenv("ULI_DEBIAN_SUITE", suite.c_str(), 1);
                    if (!state.active_mirrors.empty() && state.active_mirrors[0] != "Default") {
                        setenv("ULI_DEBIAN_MIRROR", state.active_mirrors[0].c_str(), 1);
                    }
                }

                std::string command = pm->build_install_command(final_pkgs);
                std::cout << "\n[INFO] Executing Final Installation Command: " << command << std::endl;
                BlackBox::log("EXEC_INSTALL: " + command);

                // Perform supervised installation
                SupervisionResult s_res = ProcessSupervisor::execute(command);
                
                if (s_res == SupervisionResult::RESTART_SOFT) {
                    Warn::print_warning("INTERRUPT: Soft Restart Requested. Redoing package installation...");
                    retry_packages = true;
                    continue;
                } else if (s_res == SupervisionResult::RESTART_HARD) {
                    Warn::print_warning("INTERRUPT: Hard Restart Requested. Redoing full installation from format stage...");
                    uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
                    break; // Break soft loop to restart hard loop
                } else if (s_res == SupervisionResult::ABORT) {
                    Warn::print_warning("\n\033[1;31m!! INSTALLATION ABORTED BY USER !!\033[0m");
                    Warn::print_info("Cleaning up mounts and exiting...");
                    uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
                    exit(0);
                } else if (s_res == SupervisionResult::FAILURE) {
                    Warn::print_error("Installation command failed! Check logs for details.");
                    uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
                    return;
                }

                // 4. Generate fstab (Arch Specific)
                if (os_distro == "Arch Linux") {
                    if (!uli::runtime::UIHandler::generate_fstab("/mnt")) {
                        Warn::print_error("Failed to generate /etc/fstab!");
                    }
                }

                // 5. Generate requested Locales 
                if (!state.locale_language.empty() && !state.locale_encoding.empty()) {
                    Warn::print_info("Executing Locale Generation: " + state.locale_language + "." + state.locale_encoding);
                    uli::localegen::LocaleGenerator::generate_locales(state.drive, state.locale_language, state.locale_encoding);
                }

                // 6. Finalize System Configuration (Hostname, Users, Bootloader)
                if (!uli::runtime::PostInstaller::finalize(state, os_distro, "/mnt")) {
                    Warn::print_error("Post-installation configuration failed! System may not be bootable.");
                    uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
                    return;
                }

                Warn::print_info("Installation stage complete. Unmounting target...");
                uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
                uli::runtime::config::PersistenceManager::clear_recovery();
                Warn::print_info("All operations finished. You may now reboot.");
                return; // Finished everything successfully
            }
        }
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_UI_HPP
