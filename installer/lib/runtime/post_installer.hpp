#ifndef ULI_RUNTIME_POST_INSTALLER_HPP
#define ULI_RUNTIME_POST_INSTALLER_HPP

#include "../chroot_hook/chroot_hook.hpp"
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
    static bool finalize(const MenuState& state, const std::string& os_distro, const std::string& target_root = "/mnt") {
        BlackBox::log("ENTER PostInstaller::finalize for " + os_distro);
        Warn::print_info("Finalizing system configuration for " + os_distro + "...");

        // 1. Prepare Environment (Bind Mounts handled by RAII ScopedChroot)
        uli::hook::UniversalChroot::ScopedChroot chroot(target_root);
        if (!chroot.is_ready()) {
            Warn::print_error("Failed to prepare chroot environment.");
            return false;
        }

        bool success = true;
        if (!setup_hostname(state, target_root)) success = false;
        if (success && !setup_users(state, target_root, os_distro, chroot)) success = false;
        if (success && !setup_ntp(state, target_root, os_distro, chroot)) success = false;
        if (success && !setup_bootloader(state, target_root, os_distro, chroot)) success = false;

        if (success) BlackBox::log("EXIT PostInstaller::finalize success");
        else BlackBox::log("EXIT PostInstaller::finalize with ERRORS");
        
        return success;
    }

private:
    static bool setup_hostname(const MenuState& state, const std::string& root) {
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
            hsf << "127.0.1.1\t" << state.hostname << ".localdomain\t" << state.hostname << "\n";
            hsf.close();
        }

        return true;
    }

    static bool setup_users(const MenuState& state, const std::string& root, const std::string& os_distro, uli::hook::UniversalChroot::ScopedChroot& chroot) {
        // 1. Root Password
        if (state.root_password != "None" && !state.root_password.empty()) {
            BlackBox::log("POST-INSTALL: Setting root password");
            std::string cmd = "echo \"root:" + state.root_password + "\" | chpasswd -R " + root;
            if (std::system(cmd.c_str()) != 0) {
                BlackBox::log("ERROR: Failed to set root password");
                return false;
            }
        }

        // 2. User Accounts
        for (const auto& u : state.users) {
            BlackBox::log("POST-INSTALL: Creating user " + u.username);
            
            if (os_distro == "Alpine Linux") {
                // Alpine adduser (BusyBox)
                std::string user_cmd = "adduser -D " + u.username;
                if (chroot.execute(user_cmd) != 0) {
                    BlackBox::log("ERROR: Failed to adduser " + u.username);
                    return false;
                }
            } else {
                // Arch/Debian useradd
                std::string groups = "video,audio,optical,storage";
                if (u.is_wheel) groups += ",wheel";
                std::string user_cmd = "useradd -m -G " + groups + " " + u.username;
                if (chroot.execute(user_cmd) != 0) {
                    BlackBox::log("ERROR: Failed to useradd " + u.username);
                    return false;
                }
            }

            // Set password
            std::string pass_cmd = "echo \"" + u.username + ":" + u.password + "\" | chpasswd -R " + root;
            if (std::system(pass_cmd.c_str()) != 0) {
                BlackBox::log("ERROR: Failed to set password for " + u.username);
                return false;
            }

            // Sudo/Wheel permissions
            if (u.is_wheel || u.is_sudoer) {
                if (os_distro == "Alpine Linux") {
                    chroot.execute("addgroup " + u.username + " wheel");
                } else if (os_distro == "Debian") {
                    chroot.execute("usermod -aG sudo " + u.username);
                } else {
                    chroot.execute("usermod -aG wheel " + u.username);
                }

                // Ensure wheel group has sudo access
                std::string sudo_sed = "sed -i 's/^# %wheel ALL=(ALL:ALL) ALL/%wheel ALL=(ALL:ALL) ALL/' " + root + "/etc/sudoers";
                std::system(sudo_sed.c_str());
            }
        }

        return true;
    }

    static bool setup_ntp(const MenuState& state, const std::string& root, const std::string& os_distro, uli::hook::UniversalChroot::ScopedChroot& chroot) {
        if (!state.ntp_sync) return true;
        
        BlackBox::log("POST-INSTALL: Configuring NTP");
        if (os_distro == "Alpine Linux") {
            chroot.execute("apk add chrony");
            chroot.execute("rc-update add chronyd default");
        } else {
            // Arch/Debian systemd-timesyncd
            std::string ntp_conf = root + "/etc/systemd/timesyncd.conf";
            std::ofstream nf(ntp_conf, std::ios::app);
            if (nf.is_open()) {
                nf << "\n[Time]\nNTP=" << state.ntp_server << "\n";
                nf.close();
            }
            chroot.execute("systemctl enable systemd-timesyncd");
        }
        return true;
    }

    static bool setup_bootloader(const MenuState& state, const std::string& root, const std::string& os_distro, uli::hook::UniversalChroot::ScopedChroot& chroot) {
        BlackBox::log("POST-INSTALL: Installing bootloader " + state.bootloader);
        
        if (os_distro == "Alpine Linux") {
            if (state.bootloader == "grub") {
                chroot.execute("apk add grub grub-efi efibootmgr");
                chroot.execute("grub-install --target=" + state.bootloader_target + " --bootloader-id='" + state.bootloader_id + "' --efi-directory=" + state.efi_directory);
                chroot.execute("grub-mkconfig -o /boot/grub/grub.cfg");
            } else if (state.bootloader == "syslinux") {
                chroot.execute("apk add syslinux");
                // Note: syslinux setup is complex on Alpine, but we initiate it here
                chroot.execute("setup-bootable /boot " + state.drive);
            }
        } else if (os_distro == "Debian") {
            if (state.bootloader == "grub") {
                chroot.execute("apt-get update");
                chroot.execute("apt-get install -y grub-efi efibootmgr");
                chroot.execute("grub-install --target=" + state.bootloader_target);
                chroot.execute("update-grub");
            }
        } else {
            // Arch Linux
            if (state.bootloader == "systemd-boot") {
                chroot.execute("bootctl install");
            } else if (state.bootloader == "grub") {
                chroot.execute("grub-install --target=" + state.bootloader_target + " --bootloader-id='" + state.bootloader_id + "' --efi-directory=" + state.efi_directory);
                chroot.execute("grub-mkconfig -o /boot/grub/grub.cfg");
            }
        }

        return true;
    }
};

} // namespace runtime
} // namespace uli

#endif
