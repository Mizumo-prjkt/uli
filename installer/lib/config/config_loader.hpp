#ifndef ULI_RUNTIME_CONFIG_LOADER_HPP
#define ULI_RUNTIME_CONFIG_LOADER_HPP

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include "../runtime/menu.hpp"
#include "../runtime/warn.hpp"
#include <yaml-cpp/yaml.h>

namespace uli {
namespace runtime {
namespace config {

class ConfigLoader {
public:
    static bool load_yaml_to_menu_state(const std::string& filepath, MenuState& state) {
        try {
            YAML::Node config = YAML::LoadFile(filepath);
            
            if (config["language"]) state.language = config["language"].as<std::string>();
            if (config["current_stage"]) state.current_stage = config["current_stage"].as<std::string>();
            if (config["keyboard_layout"]) state.keyboard_layout = config["keyboard_layout"].as<std::string>();

            if (config["drive"]) state.drive = config["drive"].as<std::string>();
            if (config["bootloader"]) state.bootloader = config["bootloader"].as<std::string>();
            if (config["bootloader_target"]) state.bootloader_target = config["bootloader_target"].as<std::string>();
            if (config["bootloader_id"]) state.bootloader_id = config["bootloader_id"].as<std::string>();
            if (config["efi_directory"]) state.efi_directory = config["efi_directory"].as<std::string>();
            if (config["efi_partition"]) state.efi_partition = config["efi_partition"].as<int>();
            if (config["hostname"]) state.hostname = config["hostname"].as<std::string>();
            if (config["root_password"]) state.root_password = config["root_password"].as<std::string>();
            if (config["profile"]) state.profile = config["profile"].as<std::string>();
            if (config["audio"]) state.audio = config["audio"].as<std::string>();
            if (config["kernel"]) state.kernel = config["kernel"].as<std::string>();
            if (config["network_configuration"]) state.network_configuration = config["network_configuration"].as<std::string>();
            if (config["network_backend"]) state.network_backend = config["network_backend"].as<std::string>();
            if (config["wifi_ssid"]) state.wifi_ssid = config["wifi_ssid"].as<std::string>();
            if (config["wifi_passphrase"]) state.wifi_passphrase = config["wifi_passphrase"].as<std::string>();
            if (config["timezone"]) state.timezone = config["timezone"].as<std::string>();
            if (config["rtc_in_utc"]) state.rtc_in_utc = config["rtc_in_utc"].as<bool>();
            if (config["ntp_sync"]) state.ntp_sync = config["ntp_sync"].as<bool>();
            if (config["ntp_server"]) state.ntp_server = config["ntp_server"].as<std::string>();
            if (config["debian_version"]) state.debian_version = config["debian_version"].as<int>();
            
            if (config["arch_repo_core"]) state.arch_repo_core = config["arch_repo_core"].as<bool>();
            if (config["arch_repo_extra"]) state.arch_repo_extra = config["arch_repo_extra"].as<bool>();
            if (config["arch_repo_multilib"]) state.arch_repo_multilib = config["arch_repo_multilib"].as<bool>();
            
            if (config["alpine_repo_main"]) state.alpine_repo_main = config["alpine_repo_main"].as<bool>();
            if (config["alpine_repo_community"]) state.alpine_repo_community = config["alpine_repo_community"].as<bool>();
            if (config["alpine_repo_testing"]) state.alpine_repo_testing = config["alpine_repo_testing"].as<bool>();

            if (config["optional_repos"] && config["optional_repos"].IsSequence()) {
                state.optional_repos.clear();
                for (const auto& node : config["optional_repos"]) {
                    state.optional_repos.push_back(node.as<std::string>());
                }
            }

            if (config["additional_packages"] && config["additional_packages"].IsSequence()) {
                state.additional_packages.clear();
                for (const auto& node : config["additional_packages"]) {
                    state.additional_packages.push_back(node.as<std::string>());
                }
            }

            if (config["disk_partitions"] && config["disk_partitions"].IsSequence()) {
                state.partitions.clear();
                for (const auto& p_node : config["disk_partitions"]) {
                    PartitionConfig part;
                    if (p_node["device"]) part.device_path = p_node["device"].as<std::string>();
                    if (p_node["fs_type"]) part.fs_type = p_node["fs_type"].as<std::string>();
                    if (p_node["label"]) part.label = p_node["label"].as<std::string>();
                    if (p_node["mount_options"]) part.mount_options = p_node["mount_options"].as<std::string>();
                    if (p_node["size_cmd"]) {
                        part.size_cmd = p_node["size_cmd"].as<std::string>();
                        part.is_deferred = true; 
                    }
                    if (p_node["type_code"]) part.type_code = p_node["type_code"].as<std::string>();
                    if (p_node["part_num"]) part.part_num = p_node["part_num"].as<int>();

                    // Normalize mount point late to ensure fs_type is already known
                    if (p_node["mount_point"]) {
                        std::string mp = p_node["mount_point"].as<std::string>();
                        if (part.fs_type == "swap" || mp == "[SWAP]" || mp == "swap" || mp == "none") {
                            part.mount_point = "";
                        } else {
                            part.mount_point = mp;
                        }
                    }
                    
                    state.partitions.push_back(part);
                }

                // Auto-derive state.drive from partition device entries if not explicitly set
                if (state.drive.empty() && !state.partitions.empty()) {
                    for (const auto& p : state.partitions) {
                        if (!p.device_path.empty()) {
                            state.drive = p.device_path;
                            break;
                        }
                    }
                }

                // If we have deferred partitions from YAML, assume purge intent
                bool has_deferred = false;
                for (const auto& p : state.partitions) {
                    if (p.is_deferred) { has_deferred = true; break; }
                }
                if (has_deferred) state.purge_disk_intent = true;
            }

            if (config["users"] && config["users"].IsSequence()) {
                state.users.clear();
                for (const auto& u_node : config["users"]) {
                    UserConfig user;
                    if (u_node["username"]) user.username = u_node["username"].as<std::string>();
                    if (u_node["password"]) user.password = u_node["password"].as<std::string>();
                    if (u_node["full_name"]) user.full_name = u_node["full_name"].as<std::string>();
                    if (u_node["is_sudoer"]) user.is_sudoer = u_node["is_sudoer"].as<bool>();
                    if (u_node["is_wheel"]) user.is_wheel = u_node["is_wheel"].as<bool>();
                    if (u_node["create_home"]) user.create_home = u_node["create_home"].as<bool>();
                    state.users.push_back(user);
                }
            }
            
            if (config["custom_translations"] && config["custom_translations"].IsSequence()) {
                state.manual_mappings.clear();
                for (const auto& m_node : config["custom_translations"]) {
                    ManualMapping mapping;
                    if (m_node["generic"]) mapping.generic_name = m_node["generic"].as<std::string>();
                    if (m_node["arch"]) mapping.arch_pkg = m_node["arch"].as<std::string>();
                    if (m_node["alpine"]) mapping.alpine_pkg = m_node["alpine"].as<std::string>();
                    if (m_node["debian"]) mapping.debian_pkg = m_node["debian"].as<std::string>();
                    state.manual_mappings.push_back(mapping);
                }
            }

            return true;
        } catch (const YAML::Exception& e) {
            std::cerr << "[ERROR] YAML Syntax/Parsing Error in " << filepath << ": " << e.what() << std::endl;
            return false;
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Unexpected error loading " << filepath << ": " << e.what() << std::endl;
            return false;
        }
    }
};

} // namespace config
} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_CONFIG_LOADER_HPP
