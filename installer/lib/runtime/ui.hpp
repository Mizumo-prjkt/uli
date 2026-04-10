#ifndef ULI_RUNTIME_UI_HPP
#define ULI_RUNTIME_UI_HPP

#include "../config/config_loader.hpp"
#include "../config/persistence_manager.hpp"
#include "process_supervisor.hpp"

#include "dependency_check.hpp"
#include "design_ui.hpp"
#include "dialogbox.hpp"
#include "handler.hpp"
#include "network_metrics.hpp"
#include "post_installer.hpp"
#include "warn.hpp"
#include <iostream>
#include <string>

#include "../config/config_exporter.hpp"
#include "../efi_check/efi_check.hpp"
#include "../localegen/localegen.hpp"
#include "../network_mgr/connect.hpp"
#include "../package_mgr/apk/mirrorlist.hpp"
#include "../package_mgr/dpkg_apt/mirrorlist_debian.hpp"
#include "../timedate/ui_tz_select.hpp"
#include "contents/kb_layout/kblayout.hpp"
#include "contents/locale/locale_encode.hpp"
#include "contents/locale/locale_lang.hpp"
#include "contents/profiles/profiles.cpp"
#include "contents/repositories/repo_config.hpp"
#include "contents/repositories/repo_validator.hpp"
#include "contents/translations/english.hpp"
#include "contents/translations/filipino.hpp"
#include "contents/translations/french.hpp"
#include "contents/translations/german.hpp"
#include "contents/translations/italian.hpp"
#include "contents/translations/lang_export.hpp"
#include "contents/translations/spanish.hpp"
#include "contents/user/user_processing.hpp"
#include "selection_ui.hpp"
#include "ui_multiselect.hpp"
namespace uli {
namespace runtime {

class UIManager {
public:
  static void start_ui(const std::string &os_distro, int debian_version = 0,
                       const MenuState &initial_state = MenuState(),
                       bool load_last_error = false) {
    MenuState state = initial_state;

    // 0. Emergency Recovery / Checkpoint Logic
    if (load_last_error || uli::runtime::config::PersistenceManager::has_checkpoint()) {
      bool do_recovery = load_last_error;
      if (!do_recovery) {
        do_recovery = DialogBox::ask_yes_no(
              _tr("Checkpoint Detected"),
              _tr("An interrupted installation was detected. Would you like to "
                  "resume where you left off?"));
      }

      if (do_recovery) {
        if (uli::runtime::config::PersistenceManager::load_checkpoint(state)) {
           Warn::print_info("Resuming from stage: " + state.current_stage);
        } else {
           Warn::print_error("Failed to load checkpoint file.");
        }
      }
    }

    // Set defaults for specific distros
    if (os_distro == "Debian") {
        state.bootloader = "grub";
        state.efi_directory = "/boot/efi";
    }



    if (NetworkMetrics::run_diagnostics()) {
      state.network_configuration = "Connected";
    } else {
      DialogBox::show_alert(
          _tr("No Internet Connection"),
          _tr("Warning: Running the installation requires an active internet "
              "connection to download packages.\n\nYou can configure the "
              "network later in the installer options."));
      state.network_configuration = "Not configured";
    }

    int selected_index = 0;
    while (true) {
      std::string action =
          SelectionUI::render_main_menu(state, selected_index, os_distro);
      if (action != "NONE") {
        // Keep cursor memory internally managed by reference
      }

      if (action == "ABORT") {
        Warn::print_warning("Installation Aborted.");
        uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
        return;
      } else if (action == "INSTALL") {
        std::string missing = "";
        if (state.drive.empty() || state.drive == "None")
          missing += "Drive, ";
        if (state.root_password == "None" || state.root_password.empty())
          missing += "Root Password, ";
        if (state.users.empty())
          missing += "User Account, ";
        if (state.profile == "None" || state.profile.empty())
          missing += "Profile, ";
        if (state.network_configuration == "Not configured" ||
            state.network_configuration.empty())
          missing += "Network config, ";

        if (!missing.empty()) {
          missing.pop_back();
          missing.pop_back(); // Remove trailing comma
          DialogBox::show_alert(_tr("Installation Blocked"),
                                _tr("Missing required configs: ") + missing);
          continue;
        }

        if (DialogBox::ask_yes_no(_tr("Confirm Installation"),
                                  _tr("Begin formatting and installing to ") +
                                      state.drive +
                                      _tr("? This action is destructive!"))) {
          break;
        }
      } else if (action == "SAVE") {
        std::string path = "uli_config_dump.yaml";
        if (uli::runtime::config::ConfigExporter::save_menu_state_to_yaml(
                state, path)) {
          DialogBox::show_alert(_tr("Success"),
                                _tr("Configuration saved successfully to: ") +
                                    path);
        }
      } else if (action == "LANG") { // Installation Language
        std::vector<std::string> langs = {"English", "Filipino", "Spanish",
                                          "French",  "German",   "Italian"};
        int res = DialogBox::ask_selection(
            _tr("Language"), _tr("Select display language"), langs);
        if (res != -1) {
          state.language = langs[res];
          if (langs[res] == "Filipino") {
            uli::runtime::contents::translations::active_dict =
                uli::runtime::contents::translations::get_filipino_dict();
          } else if (langs[res] == "Spanish") {
            uli::runtime::contents::translations::active_dict =
                uli::runtime::contents::translations::get_spanish_dict();
          } else if (langs[res] == "French") {
            uli::runtime::contents::translations::active_dict =
                uli::runtime::contents::translations::get_french_dict();
          } else if (langs[res] == "German") {
            uli::runtime::contents::translations::active_dict =
                uli::runtime::contents::translations::get_german_dict();
          } else if (langs[res] == "Italian") {
            uli::runtime::contents::translations::active_dict =
                uli::runtime::contents::translations::get_italian_dict();
          } else {
            uli::runtime::contents::translations::active_dict =
                uli::runtime::contents::translations::get_english_dict();
          }
        }
      } else if (action == "KBD") { // Keyboard Layout
        std::vector<std::string> layouts = uli::runtime::contents::kb_layout::
            KBLayout::get_supported_layouts();
        int res = DialogBox::ask_selection(
            _tr("Keyboard layout"), _tr("Select standard keyboard ISO layout"),
            layouts);
        if (res != -1)
          state.keyboard_layout = layouts[res];
      } else if (action == "MIRROR") { // Mirror Region
        if (os_distro == "Arch Linux") {
          std::vector<std::string> opts = {
              _tr("Auto-Optimize (Reflector)"),
              _tr("Custom Mirror URL (Manual Entry)")};
          int res = DialogBox::ask_selection(
              _tr("Mirror Region"), _tr("Select Arch Linux mirror behavior"),
              opts);
          if (res == 0) {
            state.active_mirrors = {"Auto-Optimize (Reflector)"};
          } else if (res == 1) {
            std::string url = DialogBox::ask_input(
                _tr("Mirror Settings"), _tr("Enter full mirror URL. Ref: "
                                            "https://archlinux.org/mirrors/"));
            if (!url.empty())
              state.active_mirrors = {url};
          }
        } else if (os_distro == "Debian" || os_distro == "Alpine Linux") {
          std::vector<std::string> initial_opts = {
              _tr("Use Default Mirrors Only"), _tr("Select custom mirrors...")};
          int initial_res = DialogBox::ask_selection(
              _tr("Mirror Region"),
              _tr("Do you want to use the default repositories or select "
                  "specific mirrors?"),
              initial_opts);

          if (initial_res == 0) {
            state.active_mirrors = {"Default"};
          } else if (initial_res == 1) {
            std::vector<std::pair<std::string, std::string>> mirrors;
            std::string ref_link;
            if (os_distro == "Debian") {
              mirrors = uli::package_mgr::dpkg_apt::MirrorlistDebian::
                  get_debian_mirrors();
              ref_link = "Ref: https://www.debian.org/mirror/list";
            } else {
              mirrors = uli::package_mgr::apk::Mirrorlist::get_alpine_mirrors();
              ref_link = "Ref: https://mirrors.alpinelinux.org/";
            }

            std::vector<std::string> display_list;
            std::unordered_map<std::string, std::string> mirror_map;

            display_list.push_back(_tr("Custom Mirror URL (Manual Entry)"));

            for (const auto &m : mirrors) {
              std::string entry = m.first + " (" + m.second + ")";
              display_list.push_back(entry);
              mirror_map[entry] = m.second;
            }

            std::vector<std::string> pre_selected;
            for (const auto &url : state.active_mirrors) {
              if (url == "Default")
                continue;
              bool found = false;
              for (const auto &kv : mirror_map) {
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

            std::vector<std::string> results =
                UIMultiSelectDesign::show_multi_selection_list(
                    "Mirror Region", display_list, pre_selected, nullptr,
                    ref_link);

            std::vector<std::string> final_mirrors;
            for (const auto &res : results) {
              if (res == _tr("Custom Mirror URL (Manual Entry)")) {
                std::string url = DialogBox::ask_input(
                    _tr("Mirror Settings"),
                    _tr("Enter full explicit mirror URL:"));
                if (!url.empty())
                  final_mirrors.push_back(url);
              } else {
                final_mirrors.push_back(mirror_map[res]);
              }
            }

            if (!final_mirrors.empty()) {
              state.active_mirrors = final_mirrors;
            }
          }
        } else {
          DialogBox::show_alert(_tr("Mirror Settings"),
                                _tr("Mirror selection is not natively "
                                    "supported for this distribution yet."));
        }
      } else if (action == "REPOS") { // Optional Repositories Hook
        uli::runtime::contents::repositories::RepoConfig::run_interactive(
            state, os_distro);
      } else if (action == "LOCALE_LANG") { // Locale Language
        std::vector<std::string> locales =
            uli::runtime::contents::locale::LocaleLang::get_languages();
        int res = DialogBox::ask_selection(_tr("Locale language"),
                                           _tr("Select locale target bindings"),
                                           locales);
        if (res != -1)
          state.locale_language = locales[res];
      } else if (action == "LOCALE_ENC") { // Locale Encoding
        std::vector<std::string> encodes =
            uli::runtime::contents::locale::LocaleEncode::get_encodings();
        int res = DialogBox::ask_selection(
            _tr("Locale encoding"), _tr("Select standard locale encoding"),
            encodes);
        if (res != -1)
          state.locale_encoding = encodes[res];
      } else if (action == "DRIVE") { // Drive Configuration
        std::string target = uli::runtime::UIHandler::handle_disk_configuration(state, os_distro);
        if (!target.empty())
          state.drive = target;
      } else if (action == "BOOT") { // Bootloader
        bool is_efi = uli::efi_check::EFICheck::is_efi_system();
        if (os_distro == "Arch Linux") {
          std::string sysd_label =
              "systemd-boot" +
              std::string(is_efi ? " (RECOMMENDED FOR YOUR SYSTEM)" : "");
          std::string grub_label =
              "grub" +
              std::string(!is_efi ? " (RECOMMENDED FOR YOUR SYSTEM)" : "");
          std::vector<std::string> opts = {sysd_label, grub_label};

          int res = DialogBox::ask_selection(
              _tr("Bootloader"), _tr("Select target bootloader"), opts);
          if (res != -1) {
            state.bootloader = (res == 0) ? "systemd-boot" : "grub";

            if (state.bootloader == "grub") {
              std::vector<std::string> tgts = {
                  "x86_64-efi (Requires: grub, efibootmgr)",
                  "i386-pc (Legacy BIOS) (Requires: grub)"};
              int tres = DialogBox::ask_selection(
                  _tr("Target Firmware"),
                  _tr("Select bootloader target architecture"), tgts);
              if (tres == 0)
                state.bootloader_target = "x86_64-efi";
              else if (tres == 1)
                state.bootloader_target = "i386-pc";
            } else {
              // systemd-boot only supports UEFI
              state.bootloader_target = "x86_64-efi";
            }

            std::string id = DialogBox::ask_input(
                _tr("Bootloader ID"),
                _tr("Enter bootloader ID/description (default: Arch Linux):"));
            if (!id.empty())
              state.bootloader_id = id;
            else
              state.bootloader_id = "Arch Linux";

            std::string efi = DialogBox::ask_input(
                _tr("EFI Directory"), _tr("Where is the EFI System Partition "
                                          "mounted? (default: /boot)"));
            if (!efi.empty())
              state.efi_directory = efi;
            else
              state.efi_directory = "/boot";
          }
        } else if (os_distro == "Debian") {
          DialogBox::show_alert(
              _tr("Bootloader"),
              _tr("Debian natively enforces 'grub' as the default bootloader. "
                  "Manual selection is bypassed."));
          state.bootloader = "grub";
        } else if (os_distro == "Alpine Linux") {
          std::string sysd_label =
              "syslinux" +
              std::string(!is_efi ? " (RECOMMENDED FOR YOUR SYSTEM)" : "");
          std::string grub_label =
              "grub" +
              std::string(is_efi ? " (RECOMMENDED FOR YOUR SYSTEM)" : "");
          std::vector<std::string> opts = {sysd_label, grub_label};

          int res = DialogBox::ask_selection(
              _tr("Bootloader"), _tr("Select target bootloader"), opts);
          if (res != -1) {
            state.bootloader = (res == 0) ? "syslinux" : "grub";

            std::vector<std::string> tgts;
            if (state.bootloader == "grub") {
              tgts = {"x86_64-efi (Requires: grub-efi, efibootmgr)",
                      "i386-pc (Legacy BIOS) (Requires: grub-bios)"};
            } else {
              tgts = {"x86_64-efi (Requires: syslinux, efibootmgr)",
                      "i386-pc (Legacy BIOS) (Requires: syslinux)"};
            }
            int tres = DialogBox::ask_selection(
                _tr("Target Firmware"),
                _tr("Select bootloader target architecture"), tgts);
            if (tres == 0)
              state.bootloader_target = "x86_64-efi";
            else if (tres == 1)
              state.bootloader_target = "i386-pc";

            std::string id = DialogBox::ask_input(
                _tr("Bootloader ID"), _tr("Enter bootloader ID/description "
                                          "(default: Alpine Linux):"));
            if (!id.empty())
              state.bootloader_id = id;
            else
              state.bootloader_id = "Alpine Linux";

            std::string efi = DialogBox::ask_input(
                _tr("EFI Directory"), _tr("Where is the EFI System Partition "
                                          "mounted? (default: /boot)"));
            if (!efi.empty())
              state.efi_directory = efi;
            else
              state.efi_directory = "/boot";
          }
        } else {
          DialogBox::show_alert(_tr("Bootloader"),
                                _tr("Bootloader selection is not supported for "
                                    "this distribution."));
        }
      } else if (action == "HOST") { // Hostname
        std::string host =
            DialogBox::ask_input(_tr("Hostname"), _tr("Set new hostname"));
        if (!host.empty())
          state.hostname = host;
      } else if (action == "ROOT") { // Root Password
        std::string pwd = DialogBox::ask_password(
            _tr("Root Password"),
            _tr("Set root password (stored securely in state)"));
        if (!pwd.empty())
          state.root_password = pwd;
      } else if (action == "USER") { // User Account
        uli::runtime::contents::user::UserProcessing::run_user_config(
            state, os_distro);
      } else if (action == "PROFILE") { // Profile
        std::vector<std::string> opts = {"Desktop", "Minimal", "Server",
                                         "None"};
        int res = DialogBox::ask_selection(
            _tr("Profile"),
            _tr("Select standard computing architecture experience"), opts);
        if (res != -1) {
          state.profile = opts[res];
          if (state.profile != "None") {
            uli::runtime::contents::profiles::ProfileManager::apply_profile(
                state, os_distro);
            DialogBox::show_alert(
                _tr("Profile Applied"),
                _tr("Dependencies for ") + state.profile +
                    _tr(" have been mapped to the package list."));
          } else {
            state.additional_packages.clear(); // For simplicity, wiping
                                               // previous auto-applied packages
          }
        }
      } else if (action == "AUDIO") { // Audio
        std::vector<std::string> opts = {"pipewire", "pulseaudio (legacy)",
                                         "None"};
        int res = DialogBox::ask_selection(_tr("Audio"),
                                           _tr("Select audio server"), opts);
        if (res != -1) {
          state.audio = opts[res];
        }
      } else if (action == "KERNEL") { // Kernels
        if (os_distro == "Arch Linux") {
          std::vector<std::string> opts = {"linux", "linux-zen",
                                           "linux-hardened", "linux-lts"};
          int res = DialogBox::ask_selection(
              _tr("Kernel Selection"),
              _tr("Select standard Linux kernel package:"), opts);
          if (res != -1) {
            std::string old_kernel = state.kernel;
            std::string new_kernel = opts[res];
            state.kernel = new_kernel;

            std::string old_headers = old_kernel + "-headers";
            std::string new_headers = new_kernel + "-headers";
            for (auto &pkg : state.additional_packages) {
              if (pkg == old_headers) {
                pkg = new_headers;
              }
            }
          }
        } else {
          DialogBox::show_alert(
              _tr("Kernel Selection"),
              _tr("Kernel selection is locked to the standard 'linux' package "
                  "for this distribution."));
          state.kernel = "kernel";
        }

      } else if (action == "PKGS") { // Additional packages
        state.additional_packages = UIHandler::handle_package_search(
            os_distro, state.additional_packages, state);
      } else if (action == "NET") { // Network configuration
        // Show current network status (ethernet + wireless hint)
        std::string status_report =
            uli::network_mgr::NetworkConnectHelper::build_network_status_report(
                state.connected_ssid);
        uli::runtime::DialogBox::show_alert("Network Status", status_report);

        std::vector<std::string> net_opts;
        if (uli::network_mgr::NetworkConnectHelper::has_any_ethernet()) {
          net_opts.push_back("Stay with wired connectivity");
        }
        net_opts.push_back("Configure Wireless");
        net_opts.push_back("Back");

        int net_sel = uli::runtime::DialogBox::ask_selection(
            "Network Configuration", "Choose an action:", net_opts);

        std::string choice = (net_sel >= 0 && (size_t)net_sel < net_opts.size())
                                 ? net_opts[net_sel]
                                 : "";

        if (choice == "Stay with wired connectivity") {
          state.network_configuration = "Wired";
          state.connected_ssid = "";
        } else if (choice == "Configure Wireless") {
          std::string ssid_out;
          if (uli::network_mgr::NetworkConnectHelper::
                  ui_setup_wireless_connection(ssid_out)) {
            state.connected_ssid = ssid_out;
            state.network_configuration = "Wireless (SSID: " + ssid_out + ")";
          }
        }
      } else if (action == "TZ") { // Timezone
        uli::timedate::UITzSetup::run_setup_flow(state);
      } else if (action == "NTP") { // Automatic time sync (NTP)
        uli::timedate::UITzSetup::run_ntp_setup(state);
      }
      // Other indices are purposefully ignored/unimplemented for now while
      // scaffolding
    }

    execute_install(os_distro, state);
  }

  // Static helper to execute the final installation sequence
  static void execute_install(const std::string &os_distro, MenuState &state) {
    // 0. Normalize partition standards (Authority Transfer)
    uli::runtime::UIHandler::normalize_efi_mounts(state, os_distro);

    // 0. Preliminary Dependency Check

    auto dep_result =
        uli::runtime::DependencyChecker::check_essentials(os_distro);
    if (!dep_result.success) {
      std::string msg = "Critical system tools are missing for " + os_distro +
                        " installation:\n\n" + dep_result.missing_tools +
                        "\n\n" +
                        "Please install these dependencies (e.g., 'apt install "
                        "gdisk' or 'pacman -S gdisk') and restart.";
      DialogBox::show_alert("System Readiness Failure", msg);
      return;
    }

    // Cache state for checkpointing
    uli::runtime::config::PersistenceManager::save_checkpoint(state);

    while (true) { // Hard Restart Loop
      DesignUI::clear_screen();
      Warn::print_info("Installation sequence initiated for " + os_distro);
      Warn::print_info("Target Environment: " + state.drive);
      Warn::print_info("Controls: Ctrl+U (Soft), Ctrl+O (Hold), Ctrl+R (Hard), "
                       "Ctrl+H (Help Guide)");

      // 1. Disk Preparation (Idempotent)
      if (state.current_stage == "INITIAL" || state.current_stage == "START_FORMATTING") {
        state.current_stage = "START_FORMATTING";
        uli::runtime::config::PersistenceManager::save_checkpoint(state);

        if (!uli::runtime::UIHandler::execute_deferred_partitions(state)) {
          Warn::print_error("Halting Installation: Disk Format failed.");
          uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
          return;
        }

        if (!uli::runtime::UIHandler::mount_all_partitions(state, os_distro)) {
          Warn::print_error("Halting Installation: Mounting failed.");
          uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
          return;
        }

        state.current_stage = "DISKS_PREPARED";
        uli::runtime::config::PersistenceManager::save_checkpoint(state);
      } else {
        Warn::print_info("Stage DISKS_PREPARED already reached. Ensuring mounts...");
        // Fast-path: even if disks are formatted, we must ensure they are mounted in /mnt
        if (!uli::runtime::UIHandler::mount_all_partitions(state, os_distro)) {
           Warn::print_error("Failed to re-assert mounts during resumption.");
        }
      }

      // 2.5 Ensure arch-chroot is available
      if (os_distro == "Debian") {
        if (std::system("command -v arch-chroot > /dev/null 2>&1") != 0) {
          if (std::system("apt install arch-install-scripts > /dev/null 2>&1") != 0) {
            setenv("ULI_DEBIAN_RAW_CHROOT", "1", 1);
          }
        }
      }

      bool retry_packages = true;
      while (retry_packages) {  // Soft Restart Loop
        retry_packages = false; 

        if (state.current_stage == "DISKS_PREPARED" || state.current_stage == "START_PACKAGES") {
          state.current_stage = "START_PACKAGES";
          uli::runtime::config::PersistenceManager::save_checkpoint(state);

          auto final_pkgs = uli::runtime::UIHandler::refine_package_list(os_distro, state);
          std::unique_ptr<uli::package_mgr::PackageManagerInterface> pm;
          if (os_distro == "Arch Linux") {
            pm = std::make_unique<uli::package_mgr::alps::AlpsManager>();
            pm->load_translation_from_string(uli::package_mgr::BUILTIN_TRANS_ARCH);
          } else if (os_distro == "Alpine Linux") {
            pm = std::make_unique<uli::package_mgr::apk::ApkManager>();
            pm->load_translation_from_string(uli::package_mgr::BUILTIN_TRANS_ALPINE);
          } else {
            pm = std::make_unique<uli::package_mgr::DpkgAptManager>();
            pm->load_translation_from_string(uli::package_mgr::BUILTIN_TRANS_DEBIAN);
          }

          setenv("ULI_BOOTSTRAP", "1", 1);
          if (os_distro == "Debian") {
            std::string suite = (state.debian_version == 12) ? "bookworm" : "trixie";
            setenv("ULI_DEBIAN_SUITE", suite.c_str(), 1);
          }

          std::string command = pm->build_install_command(final_pkgs);
          SupervisionResult s_res;
          if (os_distro == "Debian") {
            int res = std::system(command.c_str());
            s_res = (res == 0) ? SupervisionResult::SUCCESS : SupervisionResult::FAILURE;
          } else {
            s_res = ProcessSupervisor::execute(command);
          }

          if (s_res == SupervisionResult::RESTART_SOFT) {
            retry_packages = true;
            continue;
          } else if (s_res == SupervisionResult::RESTART_HARD) {
            state.current_stage = "INITIAL"; // Reset for hard redo
            uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
            break; 
          } else if (s_res == SupervisionResult::ABORT) {
            uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
            exit(0);
          } else if (s_res == SupervisionResult::FAILURE) {
            uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
            return;
          }

          state.current_stage = "PACKAGES_INSTALLED";
          uli::runtime::config::PersistenceManager::save_checkpoint(state);
        }

        // 4. Finalization (Idempotent)
        if (state.current_stage == "PACKAGES_INSTALLED" || state.current_stage == "START_FINALIZING") {
          state.current_stage = "START_FINALIZING";
          uli::runtime::config::PersistenceManager::save_checkpoint(state);

          if (os_distro == "Arch Linux" || os_distro == "Debian" || os_distro == "Alpine Linux") {
            uli::runtime::UIHandler::generate_fstab("/mnt");
          }

          if (!state.locale_language.empty() && !state.locale_encoding.empty()) {
            uli::localegen::LocaleGenerator::generate_locales(state.drive, state.locale_language, state.locale_encoding);
          }

          if (!uli::runtime::PostInstaller::finalize(state, os_distro, "/mnt")) {
            uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
            return;
          }

          state.current_stage = "FINALIZED";
          uli::runtime::config::PersistenceManager::save_checkpoint(state);
        }
        
        Warn::print_info("Installation stage complete. Unmounting target...");
        uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
        uli::runtime::config::PersistenceManager::clear_checkpoint();
        Warn::print_info("All operations finished. You may now reboot.");
        return; // Finished everything successfully
      }

    }
  }

  // Orchestrates the hotpatching of the bootloader in an existing system

  static bool execute_repair_workflow(MenuState &state, const std::string &os_distro) {
    Warn::print_info("Starting Bootloader Repair Mode (Hotpatch)...");
    BlackBox::log("ENTER execute_repair_workflow for " + os_distro);

    // 1. Safety: Protect existing partitions from any accidental formatting
    for (auto &p : state.partitions) p.is_deferred = false;

    // 2. Pre-flight checks: verify block devices and sizes
    if (!uli::runtime::UIHandler::verify_partitions_exist(state)) return false;

    // 3. Normalize paths according to distro standards (/boot/efi for Debian)
    uli::runtime::UIHandler::normalize_efi_mounts(state, os_distro);

    // 4. Mount target filesystems to /mnt
    std::cout << "[repair] Mounting filesystems from YAML...\n";
    if (!uli::runtime::UIHandler::mount_all_partitions(state, os_distro)) {
        Warn::print_error("Failed to mount existing filesystems for repair.");
        uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
        return false;
    }

    if (!uli::runtime::UIHandler::mount_api_systems("/mnt")) {
        Warn::print_error("Failed to mount API virtual filesystems (proc/sys/dev).");
        uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
        return false;
    }

    // 5. Enter Chroot & execute bootloader setup
    try {
        uli::hook::UniversalChroot::ScopedChroot chroot("/mnt");
        
        std::cout << "[repair] Syncing package repositories...\n";
        if (os_distro == "Debian") {
            chroot.execute("apt-get update");
        } else if (os_distro == "Arch Linux") {
            chroot.execute("pacman -Sy");
        }

        std::cout << "[repair] Re-installing bootloader with hardened flags...\n";
        if (!uli::runtime::PostInstaller::setup_bootloader(state, "/mnt", os_distro, chroot)) {
            Warn::print_error("Bootloader repair failed internally. See logs.");
            uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
            return false;
        }
    } catch (const std::exception &e) {
        Warn::print_error("Chroot fatal error: " + std::string(e.what()));
        uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
        return false;
    }

    // 6. Final cleanup (umount)
    uli::runtime::UIHandler::cleanup_mounts(state, os_distro);
    Warn::print_info("SUCCESS: Bootloader successfully repaired and hotpatched!");
    return true;
  }

  // Specialized entry point for bootloader hotpatching via --boot-repair
  static void start_repair_mode(const std::string &os_distro, const std::string &yaml_path) {
    MenuState state;
    Warn::print_info("Loading repair profile: " + yaml_path);
    if (!uli::runtime::config::ConfigLoader::load_yaml_to_menu_state(yaml_path, state)) {
        Warn::print_error("Failed to parse repair profile. Aborting.");
        return;
    }

    std::string banner = _tr("This will attempt to repair the bootloader on your existing system.\n") +
                         _tr("Drive: ") + state.drive + "\n" +
                         _tr("Distribution: ") + os_distro + "\n\n" +
                         _tr("CAUTION: Ensure your partition mapping in ") + yaml_path + _tr(" is accurate.");

    if (DialogBox::ask_yes_no(_tr("Confirm Bootloader Repair"), banner)) {
        DesignUI::clear_screen();
        if (execute_repair_workflow(state, os_distro)) {
            DialogBox::show_alert(_tr("Repair Successful"), _tr("Bootloader has been hotpatched. You may now reboot."));
        } else {
            DialogBox::show_alert(_tr("Repair Failed"), _tr("The repair process encountered an error. Check logs."));
        }
    }
  }
};



} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_UI_HPP
