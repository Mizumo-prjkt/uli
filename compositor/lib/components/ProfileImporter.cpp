#include "ProfileImporter.hpp"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <QDebug>

namespace uli {
namespace compositor {

bool ProfileImporter::importFromYaml(const QString &path, ProfileSpec &spec) {
    try {
        YAML::Node config = YAML::LoadFile(path.toStdString());
        
        if (config["language"]) spec.language = QString::fromStdString(config["language"].as<std::string>());
        if (config["hostname"]) spec.hostname = QString::fromStdString(config["hostname"].as<std::string>());
        if (config["timezone"]) spec.timezone = QString::fromStdString(config["timezone"].as<std::string>());
        if (config["root_password"]) spec.rootPassword = QString::fromStdString(config["root_password"].as<std::string>());
        if (config["bootloader"]) spec.bootTarget = QString::fromStdString(config["bootloader"].as<std::string>());
        if (config["bootloader_id"]) spec.bootloaderId = QString::fromStdString(config["bootloader_id"].as<std::string>());
        if (config["efi_directory"]) spec.efiDirectory = QString::fromStdString(config["efi_directory"].as<std::string>());
        if (config["efi_partition"]) spec.efiPart = config["efi_partition"].as<int>();
        if (config["network_backend"]) spec.networkBackend = QString::fromStdString(config["network_backend"].as<std::string>());
        if (config["wifi_ssid"]) spec.wifiSsid = QString::fromStdString(config["wifi_ssid"].as<std::string>());
        if (config["wifi_passphrase"]) spec.wifiPassphrase = QString::fromStdString(config["wifi_passphrase"].as<std::string>());
        if (config["init_system"]) spec.initSystem = QString::fromStdString(config["init_system"].as<std::string>());
        
        // Distro logic reconstruction
        if (config["arch_repo_core"]) spec.targetDistro = "Arch Linux";
        else if (config["alpine_repo_main"]) spec.targetDistro = "Alpine Linux";
        else spec.targetDistro = "Debian";

        spec.isAgnostic = (config["arch_repo_core"] && config["alpine_repo_main"]);

        // Users
        spec.users.clear();
        if (config["users"] && config["users"].IsSequence()) {
            for (const auto &unode : config["users"]) {
                ui::UserSpec user;
                if (unode["username"]) user.username = QString::fromStdString(unode["username"].as<std::string>());
                if (unode["password"]) user.password = QString::fromStdString(unode["password"].as<std::string>());
                if (unode["is_sudoer"]) user.isSudoer = unode["is_sudoer"].as<bool>();
                spec.users.append(user);
            }
        }

        // Partitions
        spec.partitions.clear();
        if (config["disk_partitions"] && config["disk_partitions"].IsSequence()) {
            for (const auto &pnode : config["disk_partitions"]) {
                storage::PartitionConfig p;
                if (pnode["fs_type"]) p.fs_type = QString::fromStdString(pnode["fs_type"].as<std::string>());
                if (pnode["label"]) p.label = QString::fromStdString(pnode["label"].as<std::string>());
                if (pnode["mount_point"]) p.mount_point = QString::fromStdString(pnode["mount_point"].as<std::string>());
                if (pnode["size_cmd"]) p.size_cmd = QString::fromStdString(pnode["size_cmd"].as<std::string>());
                if (pnode["type_code"]) p.type_code = QString::fromStdString(pnode["type_code"].as<std::string>());
                if (pnode["part_num"]) p.part_num = pnode["part_num"].as<int>();
                spec.partitions.append(p);
            }
        }

        // Packages
        spec.additionalPkgs.clear();
        if (config["additional_packages"] && config["additional_packages"].IsSequence()) {
            for (const auto &pkg : config["additional_packages"]) {
                spec.additionalPkgs.append(QString::fromStdString(pkg.as<std::string>()));
            }
        }

        // Linked Packages (Custom Translations)
        spec.linkedPkgs.clear();
        if (config["custom_translations"] && config["custom_translations"].IsSequence()) {
            for (const auto &lnode : config["custom_translations"]) {
                LinkedPackage lp;
                if (lnode["generic"]) lp.genericName = QString::fromStdString(lnode["generic"].as<std::string>());
                if (lnode["arch"]) lp.archPkg = QString::fromStdString(lnode["arch"].as<std::string>());
                if (lnode["alpine"]) lp.alpinePkg = QString::fromStdString(lnode["alpine"].as<std::string>());
                if (lnode["debian"]) lp.debianPkg = QString::fromStdString(lnode["debian"].as<std::string>());
                spec.linkedPkgs.append(lp);
            }
        }

        return true;
    } catch (...) {
        return false;
    }
}

} // namespace compositor
} // namespace uli
