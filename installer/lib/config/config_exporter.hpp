#ifndef ULI_RUNTIME_CONFIG_EXPORTER_HPP
#define ULI_RUNTIME_CONFIG_EXPORTER_HPP

#include <string>
#include <fstream>
#include <iostream>
#include "../runtime/menu.hpp"
#include "../runtime/warn.hpp"

#include <yaml-cpp/yaml.h>

namespace uli {
namespace runtime {
namespace config {

class ConfigExporter {
public:
    static bool save_menu_state_to_yaml(const MenuState& state, const std::string& filepath) {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            uli::runtime::Warn::print_error("Failed to open file for writing: " + filepath);
            return false;
        }

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "language" << YAML::Value << state.language;
        out << YAML::Key << "keyboard_layout" << YAML::Value << state.keyboard_layout;
        out << YAML::Key << "drive" << YAML::Value << state.drive;
        out << YAML::Key << "bootloader" << YAML::Value << state.bootloader;
        out << YAML::Key << "bootloader_target" << YAML::Value << state.bootloader_target;
        out << YAML::Key << "bootloader_id" << YAML::Value << state.bootloader_id;
        out << YAML::Key << "efi_directory" << YAML::Value << state.efi_directory;
        out << YAML::Key << "efi_partition" << YAML::Value << state.efi_partition;
        out << YAML::Key << "hostname" << YAML::Value << state.hostname;
        out << YAML::Key << "root_password" << YAML::Value << state.root_password;
        out << YAML::Key << "profile" << YAML::Value << state.profile;
        out << YAML::Key << "audio" << YAML::Value << state.audio;
        out << YAML::Key << "kernel" << YAML::Value << state.kernel;
        out << YAML::Key << "network_configuration" << YAML::Value << state.network_configuration;
        out << YAML::Key << "network_backend" << YAML::Value << state.network_backend;
        out << YAML::Key << "wifi_ssid" << YAML::Value << state.wifi_ssid;
        out << YAML::Key << "wifi_passphrase" << YAML::Value << state.wifi_passphrase;
        out << YAML::Key << "timezone" << YAML::Value << state.timezone;
        out << YAML::Key << "rtc_in_utc" << YAML::Value << state.rtc_in_utc;
        out << YAML::Key << "ntp_sync" << YAML::Value << state.ntp_sync;
        out << YAML::Key << "ntp_server" << YAML::Value << state.ntp_server;
        
        out << YAML::Key << "debian_version" << YAML::Value << state.debian_version;
        out << YAML::Key << "arch_repo_core" << YAML::Value << state.arch_repo_core;
        out << YAML::Key << "arch_repo_extra" << YAML::Value << state.arch_repo_extra;
        out << YAML::Key << "arch_repo_multilib" << YAML::Value << state.arch_repo_multilib;
        out << YAML::Key << "alpine_repo_main" << YAML::Value << state.alpine_repo_main;
        out << YAML::Key << "alpine_repo_community" << YAML::Value << state.alpine_repo_community;
        out << YAML::Key << "alpine_repo_testing" << YAML::Value << state.alpine_repo_testing;

        out << YAML::Key << "optional_repos" << YAML::Value << YAML::BeginSeq;
        for (const auto& repo : state.optional_repos) {
            out << repo;
        }
        out << YAML::EndSeq;
        
        out << YAML::Key << "additional_packages" << YAML::Value << YAML::BeginSeq;
        for (const auto& pkg : state.additional_packages) {
            out << pkg;
        }
        out << YAML::EndSeq;

        out << YAML::Key << "disk_partitions" << YAML::Value << YAML::BeginSeq;
        for (const auto& part : state.partitions) {
            out << YAML::BeginMap;

            // For deferred partitions, device_path is empty until execution.
            // Pre-compute it from drive + part_num for YAML export.
            std::string resolved_device = part.device_path;
            if (resolved_device.empty() && !state.drive.empty() && part.part_num > 0) {
                resolved_device = state.drive;
                if (!resolved_device.empty() && resolved_device.back() >= '0' && resolved_device.back() <= '9') {
                    resolved_device += "p" + std::to_string(part.part_num);
                } else {
                    resolved_device += std::to_string(part.part_num);
                }
            }

            out << YAML::Key << "device" << YAML::Value << resolved_device;
            out << YAML::Key << "fs_type" << YAML::Value << part.fs_type;
            out << YAML::Key << "label" << YAML::Value << part.label;
            out << YAML::Key << "mount_point" << YAML::Value << part.mount_point;
            out << YAML::Key << "mount_options" << YAML::Value << part.mount_options;
            out << YAML::Key << "size_cmd" << YAML::Value << part.size_cmd;
            out << YAML::Key << "type_code" << YAML::Value << part.type_code;
            out << YAML::Key << "part_num" << YAML::Value << part.part_num;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;

        out << YAML::Key << "users" << YAML::Value << YAML::BeginSeq;
        for (const auto& user : state.users) {
            out << YAML::BeginMap;
            out << YAML::Key << "username" << YAML::Value << user.username;
            out << YAML::Key << "password" << YAML::Value << user.password;
            out << YAML::Key << "full_name" << YAML::Value << user.full_name;
            out << YAML::Key << "is_sudoer" << YAML::Value << user.is_sudoer;
            out << YAML::Key << "is_wheel" << YAML::Value << user.is_wheel;
            out << YAML::Key << "create_home" << YAML::Value << user.create_home;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;

        out << YAML::Key << "custom_translations" << YAML::Value << YAML::BeginSeq;
        for (const auto& mapping : state.manual_mappings) {
            out << YAML::BeginMap;
            out << YAML::Key << "generic" << YAML::Value << mapping.generic_name;
            out << YAML::Key << "arch" << YAML::Value << mapping.arch_pkg;
            out << YAML::Key << "alpine" << YAML::Value << mapping.alpine_pkg;
            out << YAML::Key << "debian" << YAML::Value << mapping.debian_pkg;
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;

        out << YAML::EndMap;

        file << out.c_str() << "\n";
        file.close();
        
        return true;
    }
};

} // namespace config
} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_CONFIG_EXPORTER_HPP
