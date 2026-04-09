#include <yaml-cpp/yaml.h>
#include <sstream>
#include <fstream>
#include "ProfileExporter.hpp"
#include <QDebug>

namespace uli {
namespace compositor {

bool ProfileExporter::exportToYaml(const QString &path, const ProfileSpec &spec) {
    YAML::Node root;
    
    root["language"] = spec.language.toStdString();
    root["hostname"] = spec.hostname.toStdString();
    root["timezone"] = spec.timezone.toStdString();
    root["root_password"] = spec.rootPassword.toStdString();
    root["bootloader"] = spec.bootTarget.toStdString();
    root["bootloader_target"] = spec.isEfi ? "x86_64-efi" : "i386-pc";
    root["bootloader_id"] = spec.bootloaderId.isEmpty() ? "ULI OS" : spec.bootloaderId.toStdString();
    root["network_backend"] = spec.networkBackend.toStdString();
    root["wifi_ssid"] = spec.wifiSsid.toStdString();
    root["wifi_passphrase"] = spec.wifiPassphrase.toStdString();
    root["efi_directory"] = spec.efiDirectory.isEmpty() ? "/boot" : spec.efiDirectory.toStdString();
    if (spec.isEfi) {
        root["efi_partition"] = spec.efiPart;
    }
    root["init_system"] = spec.initSystem.toStdString();
    
    // Default values for common settings
    root["keyboard_layout"] = "us";
    root["drive"] = "/dev/sda";
    root["audio"] = "pipewire";
    root["kernel"] = "linux";
    root["network_configuration"] = "Connected";
    root["rtc_in_utc"] = true;
    root["ntp_sync"] = true;
    root["ntp_server"] = "time.google.com";

    // Set repository flags based on the target distro if not agnostic
    if (!spec.isAgnostic) {
        if (spec.targetDistro == "Arch Linux") {
            root["arch_repo_core"] = true;
            root["arch_repo_extra"] = true;
            root["arch_repo_multilib"] = false;
        } else if (spec.targetDistro == "Alpine Linux") {
            root["alpine_repo_main"] = true;
            root["alpine_repo_community"] = true;
            root["alpine_repo_testing"] = false;
        }
    } else {
        // In agnostic mode, allow broad defaults or let the installer decide
        root["arch_repo_core"] = true;
        root["alpine_repo_main"] = true;
        root["debian_version"] = 0;
    }

    // Users
    for (const auto &u : spec.users) {
        YAML::Node unode;
        unode["username"] = u.username.toStdString();
        unode["password"] = u.password.toStdString();
        unode["is_sudoer"] = u.isSudoer;
        unode["is_wheel"] = u.isSudoer;
        unode["create_home"] = true;
        root["users"].push_back(unode);
    }

    // Partitions
    for (const auto &p : spec.partitions) {
        YAML::Node pnode;
        pnode["device"] = p.device.toStdString() + QString::number(p.part_num).toStdString();
        pnode["fs_type"] = p.fs_type.toStdString();
        pnode["label"] = p.label.toStdString();
        pnode["mount_point"] = p.mount_point.toStdString();
        pnode["mount_options"] = "defaults";
        pnode["size_cmd"] = p.size_cmd.toStdString();
        pnode["type_code"] = p.type_code.toStdString();
        pnode["part_num"] = p.part_num;
        root["disk_partitions"].push_back(pnode);
    }

    // Packages
    for (const auto &pkg : spec.additionalPkgs) {
        root["additional_packages"].push_back(pkg.toStdString());
    }

    // Custom Translations (Linked Packages)
    if (!spec.linkedPkgs.isEmpty()) {
        for (const auto &lp : spec.linkedPkgs) {
            YAML::Node lnode;
            lnode["generic"] = lp.genericName.toStdString();
            if (!lp.archPkg.isEmpty()) lnode["arch"] = lp.archPkg.toStdString();
            if (!lp.alpinePkg.isEmpty()) lnode["alpine"] = lp.alpinePkg.toStdString();
            if (!lp.debianPkg.isEmpty()) lnode["debian"] = lp.debianPkg.toStdString();
            root["custom_translations"].push_back(lnode);
        }
    }

    std::ofstream fout(path.toStdString());
    if (!fout.is_open()) return false;
    fout << root;
    fout.close();
    
    return true;
}

} // namespace compositor
} // namespace uli
