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
#include <algorithm>
#include <filesystem>

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
        if (success && !setup_services(state, target_root, os_distro, chroot)) success = false;

        if (success) {
            BlackBox::log("EXIT PostInstaller::finalize success");
            if (!state.additional_packages.empty()) {
                Warn::print_info("NOTE: Some manually added packages may require manual service enablement (e.g., systemctl enable <service>).");
            }
        } else {
            BlackBox::log("EXIT PostInstaller::finalize with ERRORS");
        }
        
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
                std::string groups;
                if (os_distro == "Debian") {
                    // Debian standard groups
                    groups = "video,audio,cdrom,floppy";
                    if (u.is_wheel || u.is_sudoer) groups += ",sudo";
                } else {
                    // Arch Linux standard groups
                    groups = "video,audio,optical,storage";
                    if (u.is_wheel) groups += ",wheel";
                }

                std::string user_cmd = "useradd -m -G " + groups + " " + u.username;
                if (chroot.execute(user_cmd) != 0) {
                    BlackBox::log("ERROR: Failed to useradd " + u.username + " with groups " + groups);
                    
                    // Fallback: try without groups if the above failed (likely missing groups in minimal base)
                    BlackBox::log("RETRY: Attempting useradd without non-essential groups");
                    user_cmd = "useradd -m " + u.username;
                    if (chroot.execute(user_cmd) != 0) {
                        BlackBox::log("ERROR: Critical failure adding user " + u.username);
                        return false;
                    }
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

    static bool setup_services(const MenuState& state, const std::string& root, const std::string& os_distro, uli::hook::UniversalChroot::ScopedChroot& chroot) {
        BlackBox::log("POST-INSTALL: Configuring system services");
        bool is_openrc = (os_distro == "Alpine Linux");

        auto enable_service = [&](const std::string& name, const std::string& check_bin = "") {
            if (!check_bin.empty()) {
                std::string bin_path = root + check_bin;
                if (!std::filesystem::exists(bin_path)) return;
            }

            if (is_openrc) {
                chroot.execute("rc-update add " + name + " default");
            } else {
                chroot.execute("systemctl enable " + name);
            }
            BlackBox::log("SERVICE_ENABLED: " + name);
        };

        // 1. Networking
        if (state.network_backend == "NetworkManager") {
            enable_service(is_openrc ? "networkmanager" : "NetworkManager", "/usr/bin/NetworkManager");
        }

        // 2. Bluetooth
        bool has_bluez = false;
        for (const auto& pkg : state.additional_packages) {
            if (pkg.find("bluez") != std::string::npos) has_bluez = true;
        }
        if (has_bluez) {
            enable_service(is_openrc ? "bluetooth" : "bluetooth.service", is_openrc ? "/usr/bin/bluetoothd" : "/usr/lib/bluetooth/bluetoothd");
        }

        // 3. Display Managers
        std::vector<std::pair<std::string, std::string>> dms = {
            {"sddm", "/usr/bin/sddm"},
            {"gdm", "/usr/bin/gdm"},
            {"lightdm", "/usr/bin/lightdm"}
        };
        for (const auto& dm : dms) {
            enable_service(is_openrc ? dm.first : dm.first + ".service", dm.second);
        }

        // 4. SSH
        bool needs_ssh = (state.profile == "Server" || state.profile == "server");
        for (const auto& pkg : state.additional_packages) {
            if (pkg.find("openssh") != std::string::npos) needs_ssh = true;
        }
        if (needs_ssh) {
            enable_service(is_openrc ? "sshd" : "sshd.service", is_openrc ? "/usr/sbin/sshd" : "/usr/bin/sshd");
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
                // Ensure we use the correct metapackage for 64-bit EFI
                std::string pkg = (state.bootloader_target == "x86_64-efi") ? "grub-efi-amd64" : "grub-efi-ia32";
                chroot.execute("apt-get update");
                if (chroot.execute("apt-get install -y " + pkg + " efibootmgr") != 0) return false;
                
                std::string grub_cmd = "grub-install --target=" + state.bootloader_target + 
                                       " --bootloader-id='" + state.bootloader_id + "'" +
                                       " --efi-directory=" + state.efi_directory + 
                                       " --recheck --removable";
                
                BlackBox::log("Executing: " + grub_cmd);
                if (chroot.execute(grub_cmd) != 0) {
                    BlackBox::log("ERROR: grub-install failed");
                    return false;
                }
                if (chroot.execute("update-grub") != 0) {
                    BlackBox::log("ERROR: update-grub failed");
                    return false;
                }
            }
        }
 else {
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
