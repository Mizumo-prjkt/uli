#ifndef ULI_RUNTIME_POST_INSTALLER_HPP
#define ULI_RUNTIME_POST_INSTALLER_HPP

#include "../partitioner/format/mount_wrapper.hpp"
#include "blackbox.hpp"
#include "menu.hpp"
#include "warn.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace uli {
namespace runtime {

class PostInstaller {
public:
    static bool finalize(const MenuState& state, const std::string& target_root = "/mnt") {
        BlackBox::log("ENTER PostInstaller::finalize");
        Warn::print_info("Finalizing system configuration...");

        // Auto-detect arch-chroot
        bool has_arch_chroot = (std::system("which arch-chroot > /dev/null 2>&1") == 0);
        BlackBox::log("POST-INSTALL: has_arch_chroot=" + std::to_string(has_arch_chroot));

        // 1. Prepare Environment (Bind Mounts only if no arch-chroot)
        if (!has_arch_chroot) {
            if (!uli::partitioner::format::MountWrapper::prepare_chroot(target_root)) {
                Warn::print_error("Failed to prepare chroot environment.");
                return false;
            }
        }

        bool success = true;
        if (!setup_hostname(state, target_root)) success = false;
        if (success && !setup_users(state, target_root, has_arch_chroot)) success = false;
        if (success && !setup_ntp(state, target_root, has_arch_chroot)) success = false;
        if (success && !setup_bootloader(state, target_root, has_arch_chroot)) success = false;

        // 2. Cleanup
        if (!has_arch_chroot) {
            uli::partitioner::format::MountWrapper::cleanup_chroot(target_root);
        }

        if (success) BlackBox::log("EXIT PostInstaller::finalize success");
        else BlackBox::log("EXIT PostInstaller::finalize with ERRORS");
        
        return success;
    }

private:
    // Helper to execute a command inside chroot
    static bool exec_chroot(const std::string& root, const std::string& command, bool use_arch_chroot) {
        std::string full_cmd;
        if (use_arch_chroot) {
            full_cmd = "arch-chroot " + root + " " + command + " > /dev/null 2>&1";
        } else {
            full_cmd = "chroot " + root + " /bin/bash -c \"" + command + "\" > /dev/null 2>&1";
        }
        BlackBox::log("CHROOT_EXEC: " + command + (use_arch_chroot ? " (via arch-chroot)" : " (via chroot)"));
        return (std::system(full_cmd.c_str()) == 0);
    }

  static bool setup_hostname(const MenuState &state, const std::string &root) {
    if (state.hostname.empty() || state.hostname == "archlinux") {
      BlackBox::log("POST-INSTALL: Skipping hostname (default or empty)");
      return true;
    }

    BlackBox::log("POST-INSTALL: Setting hostname to " + state.hostname);

    // Write /etc/hostname
    std::string host_path = root + "/etc/hostname";
    std::ofstream hf(host_path);
    if (hf.is_open()) {
      hf << state.hostname << "\n";
      hf.close();
    } else {
      BlackBox::log("ERROR: Could not write " + host_path);
      return false;
    }

    // Write /etc/hosts
    std::string hosts_path = root + "/etc/hosts";
    std::ofstream hsf(hosts_path, std::ios::app);
    if (hsf.is_open()) {
      hsf << "\n127.0.0.1\tlocalhost\n";
      hsf << "::1\t\tlocalhost\n";
      hsf << "127.0.1.1\t" << state.hostname << ".localdomain\t"
          << state.hostname << "\n";
      hsf.close();
    }

    return true;
  }

    static bool setup_users(const MenuState& state, const std::string& root, bool use_arch_chroot) {
    // 1. Root Password
    if (state.root_password != "None" && !state.root_password.empty()) {
      BlackBox::log("POST-INSTALL: Setting root password");
      // chpasswd handles the chrooting logic internally with -R
      std::string cmd =
          "echo \"root:" + state.root_password + "\" | chpasswd -R " + root;
      if (std::system(cmd.c_str()) != 0) {
        BlackBox::log("ERROR: Failed to set root password");
        return false;
      }
    }

    // 2. User Accounts
    for (const auto &u : state.users) {
      BlackBox::log("POST-INSTALL: Creating user " + u.username);

      // Create user (Standard groups)
      std::string user_cmd =
          "useradd -m -G video,audio,optical,storage " + u.username;
      if (!exec_chroot(root, user_cmd, use_arch_chroot)) {
        BlackBox::log("ERROR: Failed to create user " + u.username);
        return false;
      }

      // Set password
      std::string pass_cmd = "echo \"" + u.username + ":" + u.password +
                             "\" | chpasswd -R " + root;
      if (std::system(pass_cmd.c_str()) != 0) {
        BlackBox::log("ERROR: Failed to set password for " + u.username);
        return false;
      }

      // Elevation groups
      if (u.is_wheel || u.is_sudoer) {
        exec_chroot(root, "usermod -aG wheel " + u.username, use_arch_chroot);
        BlackBox::log("POST-INSTALL: Enabling sudo/wheel for " + u.username);
        std::string sudo_sed = "sed -i 's/^# %wheel ALL=(ALL:ALL) ALL/%wheel "
                               "ALL=(ALL:ALL) ALL/' " +
                               root + "/etc/sudoers";
        std::system(sudo_sed.c_str());
      }
    }

    return true;
  }

    static bool setup_ntp(const MenuState& state, const std::string& root, bool use_arch_chroot) {
    if (!state.ntp_sync)
      return true;

    BlackBox::log("POST-INSTALL: Configuring NTP (" + state.ntp_server + ")");
    std::string ntp_conf = root + "/etc/systemd/timesyncd.conf";
    std::ofstream nf(ntp_conf, std::ios::app);
    if (nf.is_open()) {
      nf << "\n[Time]\nNTP=" << state.ntp_server << "\n";
      nf.close();
    }

    // Enable the service inside chroot
    return exec_chroot(root, "systemctl enable systemd-timesyncd", use_arch_chroot);
  }

  static bool setup_bootloader(const MenuState& state, const std::string& root, bool use_arch_chroot) {
    BlackBox::log("POST-INSTALL: Installing bootloader " + state.bootloader);

    if (state.bootloader == "systemd-boot") {
      if (!exec_chroot(root, "bootctl install", use_arch_chroot)) {
        BlackBox::log("ERROR: bootctl install failed");
        return false;
      }
    } else if (state.bootloader == "grub") {
      std::string inst_cmd =
          "grub-install --target=" + state.bootloader_target +
          " --bootloader-id=\\\"" + state.bootloader_id +
          "\\\" --efi-directory=" + state.efi_directory;
      if (!exec_chroot(root, inst_cmd, use_arch_chroot)) {
        BlackBox::log("ERROR: grub-install failed");
        return false;
      }

      if (!exec_chroot(root, "grub-mkconfig -o /boot/grub/grub.cfg", use_arch_chroot)) {
        BlackBox::log("ERROR: grub-mkconfig failed");
        return false;
      }
    }

    return true;
  }
};

} // namespace runtime
} // namespace uli

#endif
