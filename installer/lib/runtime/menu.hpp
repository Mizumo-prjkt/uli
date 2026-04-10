#ifndef ULI_RUNTIME_MENU_HPP
#define ULI_RUNTIME_MENU_HPP

#include <string>
#include <vector>

namespace uli {
namespace runtime {

struct ManualMapping {
  std::string generic_name;
  std::string arch_pkg;
  std::string alpine_pkg;
  std::string debian_pkg;
};

struct PartitionConfig {
  std::string device_path;
  std::string partuuid;
  std::string fs_type;
  std::string label;
  std::string mount_point;
  std::string mount_options = "defaults";
  
  // Deferred execution parameters
  bool is_deferred = false;
  std::string size_cmd = "";
  std::string type_code = "";
  int part_num = 0;

  // Helper to resolve the actual partition node (e.g., sda1 or nvme0n1p1)
  std::string get_real_device_path() const {
    if (device_path.empty() || part_num <= 0) return device_path;

    // Check if device ends in a digit (e.g. nvme0n1, mmcblk0)
    // In these cases, partitions are suffixed with 'p'
    char last = device_path.back();
    if (std::isdigit(last)) {
        return device_path + "p" + std::to_string(part_num);
    }
    
    // Standard SATA/IDE (e.g. sda, sdb) just append number
    return device_path + std::to_string(part_num);
  }
};


struct UserConfig {
  std::string username;
  std::string password;
  std::string full_name;
  std::string room_number;
  std::string work_phone;
  std::string home_phone;
  std::string other;
  bool is_sudoer = false;
  bool is_wheel = false;
  bool create_home = true;
};

// Houses the central state for the archinstall-like interactive installation
struct MenuState {
  std::string language = "English";
  std::string current_stage = "INITIAL";
  std::string keyboard_layout = "us";

  std::vector<std::string> active_mirrors = {"Default"};
  std::string locale_language = "en_US";
  std::string locale_encoding = "UTF-8";

  std::string drive = "";
  bool purge_disk_intent = false;
  std::vector<PartitionConfig> partitions;
  std::string bootloader = "systemd-boot";
  std::string bootloader_target = "x86_64-efi";
  std::string bootloader_id = "Arch Linux";
  std::string efi_directory = "/boot";
  int efi_partition = 1;

  std::string get_bootloader_str() const {
    if (bootloader == "grub" || bootloader == "systemd-boot" || bootloader == "syslinux" || bootloader == "limine") {
      std::string res = bootloader + " (" + bootloader_id + ") on " + efi_directory;
      if (bootloader != "syslinux") {
          res += " (Part #" + std::to_string(efi_partition) + ")";
      }
      return res;
    }
    return bootloader;
  }


  std::string hostname = "archlinux";
  std::string root_password = "None";
  std::vector<UserConfig> users;

  std::string get_users_str() const {
    if (users.empty()) return "None";
    std::string res = "[";
    for (size_t i = 0; i < users.size(); ++i) {
      res += "'" + users[i].username + "'";
      if (i < users.size() - 1) res += ", ";
    }
    res += "]";
    return res;
  }

  std::string profile = "None"; // e.g., minimal, desktop xfce
  std::string audio = "pipewire";
  std::string kernel = "linux";

  std::vector<std::string> additional_packages;
  std::string network_configuration = "Not configured";
  std::string network_backend = "NetworkManager";
  std::string wifi_ssid = "";
  std::string wifi_passphrase = "";
  std::string connected_ssid = "";  // Tracks the SSID we connected to during live install
  int debian_version = 0; // 0=auto/unknown, 12=bookworm, 13=trixie, etc.
  std::string timezone = "UTC";
  bool rtc_in_utc = true;
  bool ntp_sync = true;
  std::string ntp_server = "time.google.com";
  bool arch_repo_core = true;
  bool arch_repo_extra = true;
  bool arch_repo_multilib = false;
  bool alpine_repo_main = true;
  bool alpine_repo_community = false;
  bool alpine_repo_testing = false;
  std::vector<std::string> optional_repos;
  std::vector<ManualMapping> manual_mappings;
  bool force_sync = false;

  std::string get_optional_repos_str(const std::string& os_distro) const {
      int custom_c = optional_repos.size();
      if (os_distro == "Arch Linux") {
          int count = 0;
          if (arch_repo_core) count++;
          if (arch_repo_extra) count++;
          if (arch_repo_multilib) count++;
          if (count == 0 && custom_c == 0) return "None (WARNING: No sources)";
          return std::to_string(count) + " official, " + std::to_string(custom_c) + " custom";
      } else if (os_distro == "Debian" ) {
          return custom_c == 0 ? "None" : std::to_string(custom_c) + " custom sources";
      } else if (os_distro == "Alpine Linux") {
          int count = 0;
          if (alpine_repo_main) count++;
          if (alpine_repo_community) count++;
          if (alpine_repo_testing) count++;
          if (count == 0 && custom_c == 0) return "None (WARNING: No sources)";
          return std::to_string(count) + " official, " + std::to_string(custom_c) + " custom";
      }
      return "[]";
  }

  // Formatter helper for packages vector
  std::string get_packages_str() const {
    if (additional_packages.empty())
      return "[]";
    std::string res = "[";
    for (size_t i = 0; i < additional_packages.size(); ++i) {
      const std::string& pkg = additional_packages[i];
      bool is_linked = false;
      for (const auto& m : manual_mappings) {
        if (m.generic_name == pkg) {
          is_linked = true;
          break;
        }
      }
      
      res += "'" + pkg + (is_linked ? " [linked]" : "") + "'";
      if (i < additional_packages.size() - 1)
        res += ", ";
    }
    res += "]";
    return res;
  }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_MENU_HPP
