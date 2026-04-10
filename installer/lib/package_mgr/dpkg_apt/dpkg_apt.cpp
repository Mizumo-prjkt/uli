#include "dpkg_apt.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include "../../runtime/blackbox.hpp"

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
  std::string cmd = "find /var/lib/apt/lists -maxdepth 1 -type f | grep -v 'lock' | wc -l";
  std::string output = exec_command(cmd.c_str());
  try {
    int count = std::stoi(output);
    return count > 0;
  } catch (...) {
    return false;
  }
}

bool DpkgAptManager::sync_system() {
  std::cout << "[INFO] Synchronizing Debian package repositories (apt update)..." << std::endl;
  uli::runtime::BlackBox::log("PM_SYNC: Synchronizing Debian repositories (apt-get update)");
  int ret = std::system("apt-get update >/dev/null 2>&1");
  bool success = (ret == 0);
  if (!success) uli::runtime::BlackBox::log("PM_SYNC_FAIL: apt-get update failed");
  else uli::runtime::BlackBox::log("PM_SYNC_SUCCESS");
  return success;
}

bool DpkgAptManager::configure_mirrors(
    const std::vector<std::string> &mirror_urls) {
  if (mirror_urls.empty())
    return true;

  if (mirror_urls.size() == 1 &&
      (mirror_urls[0] == "Default" ||
       mirror_urls[0] == "http://deb.debian.org/debian/")) {
    return true;
  }

  std::string list_replace = "";
  std::string sources_replace = "";

  for (size_t i = 0; i < mirror_urls.size(); ++i) {
    std::string safe_url = mirror_urls[i];
    if (safe_url.back() != '/')
      safe_url += "/";

    if (i > 0) {
      list_replace += "\\n";
      sources_replace += " ";
    }
    list_replace += "\\1" + safe_url + "\\2";
    sources_replace += safe_url;
  }

  // Replace standard Debian repositories natively across formats
  std::string list_sed =
      "sed -i -e 's|^\\(.*\\)http://deb.debian.org/debian/\\(.*\\)|" +
      list_replace +
      "|g' "
      "/etc/apt/sources.list /etc/apt/sources.list.d/*.list 2>/dev/null";

  std::string sources_sed = "sed -i -e 's|http://deb.debian.org/debian/|" +
                            sources_replace +
                            "|g' "
                            "/etc/apt/sources.list.d/*.sources 2>/dev/null";

  int ret1 = std::system(list_sed.c_str());
  int ret2 = std::system(sources_sed.c_str());

  // We optionally run apt-get update to fetch from new mirrors immediately
  std::system("apt-get update >/dev/null 2>&1");

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
  if (!file.is_open()) return 12; // Default to bookworm
  std::string line;
  while (std::getline(file, line)) {
    if (line.find("VERSION_ID=") == 0) {
      std::string val = line.substr(11);
      if (!val.empty() && val[0] == '"') val = val.substr(1, val.size() - 2);
      try { return std::stoi(val); } catch (...) { return 12; }
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
    std::string suite = (std::getenv("ULI_DEBIAN_SUITE") ? std::string(std::getenv("ULI_DEBIAN_SUITE")) : "trixie");
    std::string mirror = (std::getenv("ULI_DEBIAN_MIRROR") ? std::string(std::getenv("ULI_DEBIAN_MIRROR")) : "http://deb.debian.org/debian/");
    
    // Phase 1: Bootstrap the base system (Creates the directory structure)
    cmd << "debootstrap " << suite << " /mnt " << mirror << " && ";
    
    // Phase 2: Setup networking (DNS) for the chroot (MUST happen after debootstrap creates /etc)
    cmd << "cp /etc/resolv.conf /mnt/etc/resolv.conf && ";

    // Phase 3: Setup policy-rc.d to prevent service hangs during install
    cmd << "printf \"#!/bin/sh\\nexit 101\\n\" > /mnt/usr/sbin/policy-rc.d && ";
    cmd << "chmod +x /mnt/usr/sbin/policy-rc.d && ";

    // Phase 4: Enable additional repository components (contrib non-free non-free-firmware)
    // Supports DEB822 (Debian 13+) and Traditional (Debian 12-) formats
    cmd << "if [ -f /mnt/etc/apt/sources.list.d/debian.sources ]; then "
        << "sed -i 's/^Components: \\(.*\\)/Components: \\1 contrib non-free non-free-firmware/' /mnt/etc/apt/sources.list.d/debian.sources; "
        << "fi; "
        << "if [ -f /mnt/etc/apt/sources.list ]; then "
        << "sed -i \"s/main\\$/main contrib non-free non-free-firmware/\" /mnt/etc/apt/sources.list; "
        << "fi && ";

    // Phase 4: chroot and install extra packages
    // If version >= 13, use 'apt', else 'apt-get'
    std::string pm_bin = (version >= 13) ? "apt" : "apt-get";
    cmd << "DEBIAN_FRONTEND=noninteractive chroot /mnt " << pm_bin << " update && ";
    cmd << "DEBIAN_FRONTEND=noninteractive chroot /mnt " << pm_bin << " install -y";

    // We'll append the packages later in the loop

    // Termination: We'll need to remove the policy file at the end.
    // However, the base class appends packages to this string.
    // I will refactor the loop to handle the cleanup.
  } else {
    cmd << config.install_cmd;
  }

  for (const auto &pkg : abstract_packages) {
    cmd << " " << translate_package(pkg);
  }

  if (is_bootstrap) {
    // Cleanup Phase (Ensures policy-rc.d is removed even if installation fails is complex here, 
    // but we add it to the success chain)
    cmd << " && rm -f /mnt/usr/sbin/policy-rc.d";
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
