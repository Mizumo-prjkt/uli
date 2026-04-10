#include "dpkg_apt.hpp"
#include "../../runtime/blackbox.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace uli {
namespace package_mgr {

DpkgAptManager::DpkgAptManager() {
  // defaults
  config.binary = "apt-get";
  config.update_cmd = "apt-get update";
  config.install_cmd = "DEBIAN_FRONTEND=noninteractive apt-get install -y";
  config.remove_cmd = "DEBIAN_FRONTEND=noninteractive apt-get remove -y";
  config.clean_cmd = "apt-get autoremove -y && apt-get clean";
}

bool DpkgAptManager::is_synced() {
  // Check if /var/lib/apt/lists/ contains metadata files (ignoring partial)
  // On a completely clean system, the directory only contains lock and partial/
  std::string cmd =
      "find /var/lib/apt/lists -maxdepth 1 -type f | grep -v 'lock' | wc -l";
  std::string output = exec_command(cmd.c_str());
  try {
    int count = std::stoi(output);
    return count > 0;
  } catch (...) {
    return false;
  }
}

bool DpkgAptManager::sync_system() {
  std::cout
      << "[INFO] Synchronizing Debian package repositories (apt update)..."
      << std::endl;
  uli::runtime::BlackBox::log(
      "PM_SYNC: Synchronizing Debian repositories (apt-get update)");
  int ret = std::system("apt-get update >/dev/null 2>&1");
  bool success = (ret == 0);
  if (!success)
    uli::runtime::BlackBox::log("PM_SYNC_FAIL: apt-get update failed");
  else
    uli::runtime::BlackBox::log("PM_SYNC_SUCCESS");
  return success;
}

bool DpkgAptManager::configure_mirrors(
    const std::vector<std::string> &mirror_urls) {
  if (mirror_urls.empty())
    return true;

  bool is_bootstrap = (std::getenv("ULI_BOOTSTRAP") != nullptr);
  std::string target_prefix = is_bootstrap ? "/mnt" : "";
  std::string suite = (std::getenv("ULI_DEBIAN_SUITE") ? std::getenv("ULI_DEBIAN_SUITE") : "bookworm");

  std::vector<std::string> verified_mirrors;
  for (const auto &url : mirror_urls) {
    if (url == "Default") continue;
    
    // Connectivity check
    std::string check_cmd = "curl -sfL --connect-timeout 5 " + url + "/dists/" + suite + "/Release >/dev/null 2>&1";
    if (std::system(check_cmd.c_str()) == 0) {
      verified_mirrors.push_back(url);
    } else {
      std::cerr << "[WARN] Mirror " << url << " is unreachable or invalid for suite " << suite << ". Skipping." << std::endl;
    }
  }

  if (verified_mirrors.empty()) {
    std::cerr << "[INFO] Using default Debian mirror (deb.debian.org)." << std::endl;
    verified_mirrors.push_back("http://deb.debian.org/debian/");
  }

  // Backup existing configurations
  std::string backup_cmd = "cd " + target_prefix + "/etc/apt && " +
                           "[ -f sources.list ] && cp -n sources.list sources.list.old; " +
                           "[ -f sources.list.d/debian.sources ] && cp -n sources.list.d/debian.sources sources.list.d/debian.oldsource; " +
                           "true";
  std::system(backup_cmd.c_str());

  std::string list_replace = "";
  std::string sources_replace = "";

  for (size_t i = 0; i < verified_mirrors.size(); ++i) {
    std::string safe_url = verified_mirrors[i];
    if (safe_url.back() != '/') safe_url += "/";

    if (i > 0) {
      list_replace += "\\n";
      sources_replace += " ";
    }
    list_replace += "\\1" + safe_url + "\\2";
    sources_replace += safe_url;
  }

  // Apply changes to sources.list (Classic)
  std::string list_sed = "sed -i -e 's|^\\(.*\\)http://deb.debian.org/debian/\\(.*\\)|" +
                         list_replace + "|g' " + target_prefix + "/etc/apt/sources.list 2>/dev/null";

  // Apply changes to debian.sources (DEB822)
  std::string sources_sed = "sed -i -e 's|http://deb.debian.org/debian/|" +
                            sources_replace + "|g' " + target_prefix + "/etc/apt/sources.list.d/*.sources 2>/dev/null";

  std::system(list_sed.c_str());
  std::system(sources_sed.c_str());

  if (is_bootstrap) {
    std::system("chroot /mnt apt-get update >/dev/null 2>&1");
  } else {
    std::system("apt-get update >/dev/null 2>&1");
  }

  return true;
}

bool DpkgAptManager::load_translation(const std::string &filepath) {
  try {
    YAML::Node yaml_doc = YAML::LoadFile(filepath);
    parse_yaml_node(yaml_doc);
    return true;
  } catch (const YAML::Exception &e) {
    std::cerr << "YAML Error loading translation dictionary " << filepath
              << ": " << e.what() << std::endl;
    return false;
  }
}

bool DpkgAptManager::load_translation_from_string(const std::string &yaml_str) {
  try {
    YAML::Node yaml_doc = YAML::Load(yaml_str);
    parse_yaml_node(yaml_doc);
    return true;
  } catch (const YAML::Exception &e) {
    std::cerr << "YAML Error loading builtin translation dictionary: "
              << e.what() << std::endl;
    return false;
  }
}

static int detect_debian_version() {
  std::ifstream file("/etc/os-release");
  if (!file.is_open())
    return 12; // Default to bookworm
  std::string line;
  while (std::getline(file, line)) {
    if (line.find("VERSION_ID=") == 0) {
      std::string val = line.substr(11);
      if (!val.empty() && val[0] == '"')
        val = val.substr(1, val.size() - 2);
      try {
        return std::stoi(val);
      } catch (...) {
        return 12;
      }
    }
  }
  return 12;
}

std::string DpkgAptManager::build_install_command(
    const std::vector<std::string> &abstract_packages) const {
  std::stringstream cmd;

  bool is_bootstrap = (std::getenv("ULI_BOOTSTRAP") != nullptr);
  if (is_bootstrap) {
    int version = detect_debian_version();
    std::string suite = (std::getenv("ULI_DEBIAN_SUITE")
                             ? std::string(std::getenv("ULI_DEBIAN_SUITE"))
                             : "trixie");
    std::string mirror = (std::getenv("ULI_DEBIAN_MIRROR")
                              ? std::string(std::getenv("ULI_DEBIAN_MIRROR"))
                              : "http://deb.debian.org/debian/");

    // Phase 1: Bootstrap the base system (Include kernel and critical tools
    // early) Moving linux-image, linux-headers here ensures they are handled
    // during base system logic
    cmd << "debootstrap --include=locales,ca-certificates " << suite << " /mnt "
        << mirror << " && ";

    // Phase 2: Setup networking (DNS) for the chroot
    cmd << "if [ -L /mnt/etc/resolv.conf ] && [ ! -e /mnt/etc/resolv.conf ]; "
           "then rm -f /mnt/etc/resolv.conf; fi; "
        << "cp /etc/resolv.conf /mnt/etc/resolv.conf && ";

    // Phase 3: Setup policy-rc.d
    cmd << "printf \"#!/bin/sh\\nexit 101\\n\" > /mnt/usr/sbin/policy-rc.d && ";
    cmd << "chmod +x /mnt/usr/sbin/policy-rc.d && ";

    // Phase 4: Configure Locales persistence (Dynamic based on user selection)
    std::string locale_lang = (std::getenv("ULI_LOCALE_LANG")
                                   ? std::string(std::getenv("ULI_LOCALE_LANG"))
                                   : "en_US");
    std::string locale_enc =
        (std::getenv("ULI_LOCALE_ENCODE")
             ? std::string(std::getenv("ULI_LOCALE_ENCODE"))
             : "UTF-8");
    std::string full_locale = locale_lang + "." + locale_enc;

    cmd << "echo 'LANG=" << full_locale << "' > /mnt/etc/default/locale && "
        << "sed -i 's/^# " << full_locale << " " << locale_enc << "/"
        << full_locale << " " << locale_enc << "/' /mnt/etc/locale.gen && ";

    // Phase 4.5: Idempotent Manual API mounts (Enforced per user request)
    // We check if the target is already a mountpoint before mounting to prevent
    // stacked mounts on retry.
    uli::runtime::BlackBox::log("DPKG_APT: Ensuring idempotent bind-mounts for "
                                "/dev, /proc, /sys, /run");
    cmd << "mountpoint -q /mnt/proc || mount -t proc /proc /mnt/proc && "
        << "mountpoint -q /mnt/sys || mount -t sysfs /sys /mnt/sys && "
        << "mountpoint -q /mnt/dev || mount --bind /dev /mnt/dev && "
        << "mountpoint -q /mnt/dev/pts || mount -t devpts devpts /mnt/dev/pts "
           "-o nosuid,noexec,relatime,gid=5,mode=620,ptmxmode=666 && "
        << "mountpoint -q /mnt/run || mount --bind /run /mnt/run && "
        << "rm -f /mnt/dev/ptmx && ln -s pts/ptmx /mnt/dev/ptmx && ";

    // Phase 5: Enable additional repository components (contrib/non-free) + Security
    cmd << "DEBUG_MIRROR=\"" << mirror << "\"; "
        << "if ! curl -sfL --connect-timeout 10 \"${DEBUG_MIRROR}/dists/" << suite << "/Release\" >/dev/null; then "
        << "  echo '[ERROR] Primary mirror unreachable. Connectivity check failed.'; "
        << "  echo '[INFO] Fallback to deb.debian.org...'; "
        << "  DEBUG_MIRROR=\"http://deb.debian.org/debian/\"; "
        << "  if ! curl -sfL --connect-timeout 10 \"${DEBUG_MIRROR}/dists/" << suite << "/Release\" >/dev/null; then "
        << "    echo '[FATAL] Total connectivity failure. No working mirrors found.'; "
        << "    echo '[HINT] Check your internet connection. You can resume with --load-last-error.'; "
        << "    exit 1; "
        << "  fi; "
        << "fi; "
        // Backups
        << "[ -f /mnt/etc/apt/sources.list ] && cp -n /mnt/etc/apt/sources.list /mnt/etc/apt/sources.list.old; "
        << "[ -f /mnt/etc/apt/sources.list.d/debian.sources ] && cp -n /mnt/etc/apt/sources.list.d/debian.sources /mnt/etc/apt/sources.list.d/debian.oldsource; "
        // Injection
        << "if [ -f /mnt/etc/apt/sources.list.d/debian.sources ]; then "
        << "  sed -i \"s|http://deb.debian.org/debian/|${DEBUG_MIRROR}|g\" /mnt/etc/apt/sources.list.d/debian.sources; "
        << "  sed -i 's/^Components: \\(.*\\)/Components: \\1 contrib non-free non-free-firmware/' /mnt/etc/apt/sources.list.d/debian.sources; "
        // Add official security repo to deb822
        << "  if ! grep -q 'security.debian.org' /mnt/etc/apt/sources.list.d/debian.sources; then "
        << "    printf \"\\nTypes: deb\\nURIs: http://security.debian.org/debian-security\\nSuites: " << suite << "-security\\nComponents: main contrib non-free non-free-firmware\\nSigned-By: /usr/share/keyrings/debian-archive-keyring.gpg\\n\" >> /mnt/etc/apt/sources.list.d/debian.sources; "
        << "  fi; "
        << "elif [ -f /mnt/etc/apt/sources.list ]; then "
        << "  if [ \"" << version << "\" -ge 13 ]; then echo '[WARN] debian.sources missing, falling back to classic sources.list'; fi; "
        << "  sed -i \"s|http://deb.debian.org/debian/|${DEBUG_MIRROR}|g\" /mnt/etc/apt/sources.list; "
        << "  sed -i \"s/main$/main contrib non-free non-free-firmware/\" /mnt/etc/apt/sources.list; "
        // Add official security repo to classic
        << "  if ! grep -q 'security.debian.org' /mnt/etc/apt/sources.list; then "
        << "    echo \"deb http://security.debian.org/debian-security " << suite << "-security main contrib non-free non-free-firmware\" >> /mnt/etc/apt/sources.list; "
        << "  fi; "
        << "else echo '[WARN] No APT sources found to patch!'; fi && ";

    // Phase 6: Consolidated Staged chroot Execution (Stage 1: Base/zstd ->
    // Stage 2: User)
    std::string pm_bin = (version >= 13) ? "apt" : "apt-get";
    std::string env = "DEBIAN_FRONTEND=noninteractive LANG=" + full_locale +
                      " DBUS_SESSION_BUS_ADDRESS=/dev/null";

    // Use standard chroot for maximum reliability as requested (Removing PTY layers).
    cmd << "chroot /mnt /bin/bash -c \"" << env << " locale-gen " << full_locale << " && "
        << env << " " << pm_bin << " update && "
        << env << " " << pm_bin << " install -y zstd linux-image-amd64 linux-headers-amd64 initramfs-tools network-manager && "
        << env << " " << pm_bin << " install -y";


    uli::runtime::BlackBox::log(
        "DPKG_APT: Manual mount bootstrap command chain initialized");

  } else {
    cmd << config.install_cmd;
  }

  for (const auto &pkg : abstract_packages) {
    cmd << " " << translate_package(pkg);
  }

  if (is_bootstrap) {
    // Phase 7: Cleanup (Using lazy unmounts since we manually bind-mounted)
    cmd << "\" && rm -f /mnt/usr/sbin/policy-rc.d && "
        << "umount -l /mnt/dev/pts /mnt/dev /mnt/proc /mnt/sys /mnt/run";
  }


  return cmd.str();
}

std::string DpkgAptManager::build_remove_command(
    const std::vector<std::string> &abstract_packages) const {
  std::stringstream cmd;
  cmd << config.remove_cmd;

  for (const auto &pkg : abstract_packages) {
    cmd << " " << translate_package(pkg);
  }

  return cmd.str();
}

bool DpkgAptManager::is_available() const {
  std::string check_cmd = "which " + config.binary + " > /dev/null 2>&1";
  int res = system(check_cmd.c_str());
  return res == 0;
}

std::vector<std::string>
DpkgAptManager::search_packages(const std::string &query) {
  std::vector<std::string> results;
  if (query.empty())
    return results;

  // Perform apt-cache search safely
  std::string cmd =
      "apt-cache search --names-only '^" + query + "' 2>/dev/null";
  std::string output = exec_command(cmd.c_str());

  if (output == "_ULI_PM_ERROR_") {
    return {"_ULI_PM_ERROR_"};
  }

  std::istringstream stream(output);
  std::string line;
  while (std::getline(stream, line)) {
    if (!line.empty()) {
      // apt-cache search returns "pkgname - description". Extract the package
      // name.
      size_t space_pos = line.find(' ');
      if (space_pos != std::string::npos) {
        results.push_back(line.substr(0, space_pos));
      } else {
        results.push_back(line);
      }
    }
  }

  return results;
}

} // namespace package_mgr
} // namespace uli
