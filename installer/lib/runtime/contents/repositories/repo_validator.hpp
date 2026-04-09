#ifndef ULI_RUNTIME_CONTENTS_REPOSITORIES_REPO_VALIDATOR_HPP
#define ULI_RUNTIME_CONTENTS_REPOSITORIES_REPO_VALIDATOR_HPP

#include "../../dialogbox.hpp"
#include "../../menu.hpp"
#include <iostream>
#include <string>
#include <vector>

namespace uli {
namespace runtime {
namespace contents {
namespace repositories {

class RepoValidator {
public:
  // Validates optional_repos entries loaded from YAML against the current
  // distro context. Called after loading a YAML config, before the interactive
  // menu loop.
  static void validate_repos(MenuState &state, const std::string &os_distro) {
    if (state.optional_repos.empty())
      return;

    // ──────────────────────────────────────────────────
    // ──────────────────────────────────────────────────
    bool has_apt_repos = false;
    for (const auto &repo : state.optional_repos) {
      if (repo.find("CLASSIC|") == 0 || repo.find("DEB822|") == 0) {
        has_apt_repos = true;
        break;
      }
    }
    if (has_apt_repos) {
      DialogBox::show_alert(
          "\033[1;33;41m ⚠ INCOMPATIBLE REPOSITORIES ⚠ \033[0m",
          "Your YAML configuration contains APT-style repository entries,\n"
          "but you are installing " +
              os_distro +
              ".\n\n"
              "These repository entries will be IGNORED.\n"
              "Only native package sources for " +
              os_distro + " are supported.");
      // Strip out APT-style repos
      std::vector<std::string> filtered;
      for (const auto &repo : state.optional_repos) {
        if (repo.find("CLASSIC|") != 0 && repo.find("DEB822|") != 0) {
          filtered.push_back(repo);
        }
      }
      state.optional_repos = filtered;
    }

    // Check for Arch-specific repos on non-Arch
    if (os_distro != "Arch Linux") {
      bool has_arch_repos = false;
      for (const auto &repo : state.optional_repos) {
        if (repo.find("Server =") != std::string::npos || repo.find("[") == 0) {
          has_arch_repos = true;
          break;
        }
      }
      if (has_arch_repos) {
        DialogBox::show_alert(
            "\033[1;33;41m ⚠ INCOMPATIBLE REPOSITORIES ⚠ \033[0m",
            "Your YAML configuration contains Arch-style repository entries,\n"
            "but you are installing " +
                os_distro +
                ".\n\n"
                "These repository entries will be IGNORED.");
        // Strip Arch-style, keep others
        std::vector<std::string> filtered;
        for (const auto &repo : state.optional_repos) {
          if (repo.find("Server =") == std::string::npos &&
              repo.find("[") != 0) {
            filtered.push_back(repo);
          }
        }
        state.optional_repos = filtered;
      }
    }

    // Check for Alpine-specific repos on non-Alpine
    if (os_distro != "Alpine Linux") {
      bool has_alpine_repos = false;
      for (const auto &repo : state.optional_repos) {
        if (repo.find("APK|") == 0 ||
            repo.find("dl-cdn.alpinelinux.org") != std::string::npos ||
            repo.find("alpine/") != std::string::npos) {
          has_alpine_repos = true;
          break;
        }
      }
      if (has_alpine_repos) {
        DialogBox::show_alert(
            "\033[1;33;41m ⚠ INCOMPATIBLE REPOSITORIES ⚠ \033[0m",
            "Your YAML configuration contains Alpine apk-style repository "
            "entries,\n"
            "but you are installing " +
                os_distro +
                ".\n\n"
                "These repository entries will be IGNORED.\n"
                "Alpine packages (.apk) are NOT compatible with other "
                "distributions.");
        std::vector<std::string> filtered;
        for (const auto &repo : state.optional_repos) {
          if (repo.find("APK|") != 0 &&
              repo.find("dl-cdn.alpinelinux.org") == std::string::npos) {
            filtered.push_back(repo);
          }
        }
        state.optional_repos = filtered;
      }
    }
    return;
  }

  // ──────────────────────────────────────────────────
  // 2. Debian version-aware format checks
  // ──────────────────────────────────────────────────
  int ver = state.debian_version;

  if (ver >= 13) {
    // Debian 13+ target but YAML has classic entries
    bool has_classic = false;
    for (const auto &repo : state.optional_repos) {
      if (repo.find("CLASSIC|") == 0) {
        has_classic = true;
        break;
      }
    }
    if (has_classic) {
      DialogBox::show_alert(
          "\033[1;33;43m ⚠ DEPRECATION WARNING ⚠ \033[0m",
          "Your YAML config contains classic sources.list entries,\n"
          "but Debian " +
              std::to_string(ver) +
              " uses the deb822 format.\n\n"
              "The classic entries WILL be applied to /etc/apt/sources.list\n"
              "for backwards compatibility, but this method is DEPRECATED\n"
              "and may stop working in future Debian releases.\n\n"
              "Consider converting your repos to deb822 format.");
      // We still keep them — just warned
    }
  } else if (ver > 0 && ver <= 12) {
    // Debian 12 or earlier with deb822 entries
    bool has_deb822 = false;
    for (const auto &repo : state.optional_repos) {
      if (repo.find("DEB822|") == 0) {
        has_deb822 = true;
        break;
      }
    }
    if (has_deb822) {
      DialogBox::show_alert(
          "\033[1;31;47m ✖ INCOMPATIBLE FORMAT ✖ \033[0m",
          "Your YAML config contains deb822 format repository entries,\n"
          "but you are targeting Debian " +
              std::to_string(ver) +
              " which does NOT\n"
              "support /etc/apt/sources.list.d/*.sources natively.\n\n"
              "These deb822 entries will be IGNORED.\n"
              "Please use classic sources.list format instead.");
      // Strip deb822 entries
      std::vector<std::string> filtered;
      for (const auto &repo : state.optional_repos) {
        if (repo.find("DEB822|") != 0)
          filtered.push_back(repo);
      }
      state.optional_repos = filtered;
    }
  }

  // ──────────────────────────────────────────────────
  // ──────────────────────────────────────────────────
  for (const auto &repo : state.optional_repos) {
    bool is_debian_url =
        (repo.find("deb.debian.org") != std::string::npos ||
         repo.find("security.debian.org") != std::string::npos);
    bool is_alpine_url =
        (repo.find("dl-cdn.alpinelinux.org") != std::string::npos ||
         repo.find("alpine/") != std::string::npos);
    bool is_arch_url =
        (repo.find("archlinux.org") != std::string::npos ||
         repo.find("mirror.rackspace.com/archlinux") != std::string::npos);

    std::string foreign_source;
    else if (os_distro == "Debian" && is_alpine_url) foreign_source =
        "Alpine Linux";
    else if (os_distro == "Debian" && is_arch_url) foreign_source =
        "Arch Linux";
    else if (os_distro == "Alpine Linux" && is_debian_url) foreign_source =
        "Debian";
    else if (os_distro == "Alpine Linux" && is_arch_url) foreign_source =
        "Arch Linux";
    else if (os_distro == "Arch Linux" && is_debian_url) foreign_source =
        "Debian";
    else if (os_distro == "Arch Linux" && is_alpine_url) foreign_source =
        "Alpine Linux";

    if (!foreign_source.empty()) {
      DialogBox::show_alert(
          "\033[1;37;41m ⚠⚠⚠ CROSS-DISTRO REPOSITORY WARNING ⚠⚠⚠ \033[0m",
          "A repository URL points to " + foreign_source +
              " but you are\n"
              "installing " +
              os_distro +
              ".\n\n"
              "\033[1;31mThis WILL cause unpredictable outcomes.\033[0m\n\n"
              "Packages from different distributions are NOT interchangeable.\n"
              "Mixing repos can break dependency resolution, corrupt\n"
              "package databases, or render the system unbootable.\n\n"
              "Without a translation dictionary (--dict), only core services\n"
              "native to " +
              os_distro +
              " will be available.\n"
              "You can still add packages manually via 'Additional "
              "packages'.\n\n"
              "\033[1;33mProceeding at your own risk.\033[0m");
      break;
    }
  }
}

// ──────────────────────────────────────────────────
// Auto-guess: attempt to suggest native equivalents
// for packages that may not exist on the target distro.
// ──────────────────────────────────────────────────
static std::vector<std::pair<std::string, std::string>>
auto_guess_packages(const std::vector<std::string> &packages,
                    const std::string &source_distro,
                    const std::string &target_distro) {
  // Common cross-distro package name mappings
  // These are rough guesses — the --dict YAML is the authoritative source
  struct Mapping {
    std::string from;
    std::string to;
    std::string src;
    std::string tgt;
  };
  static const std::vector<Mapping> guesses = {
      // Debian ↔ Arch
      {"build-essential", "base-devel", "Debian", "Arch Linux"},
      {"base-devel", "build-essential", "Arch Linux", "Debian"},
      {"base-devel", "build-essential", "Arch Linux", "Debian"},
      {"python3", "python", "Debian", "Arch Linux"},
      {"python", "python3", "Arch Linux", "Debian"},
      {"openssh-server", "openssh", "Debian", "Arch Linux"},
      {"openssh", "openssh-server", "Arch Linux", "Debian"},
      {"network-manager", "networkmanager", "Debian", "Arch Linux"},
      {"networkmanager", "network-manager", "Arch Linux", "Debian"},
      {"build-essential", "build-base", "Debian", "Alpine Linux"},
      {"build-base", "build-essential", "Alpine Linux", "Debian"},
      {"python3", "python3", "Debian", "Alpine Linux"},
      {"openssh-server", "openssh", "Debian", "Alpine Linux"},
      {"openssh", "openssh-server", "Alpine Linux", "Debian"},
      {"network-manager", "networkmanager", "Debian", "Alpine Linux"},
      {"networkmanager", "network-manager", "Alpine Linux", "Debian"},
      {"sudo", "sudo", "Debian", "Alpine Linux"},
      {"curl", "curl", "Debian", "Alpine Linux"},
      {"wget", "wget", "Debian", "Alpine Linux"},
      {"git", "git", "Debian", "Alpine Linux"},
      {"bash", "bash", "Debian", "Alpine Linux"},
      {"iptables", "iptables", "Debian", "Alpine Linux"},
      {"coreutils", "coreutils", "Debian", "Alpine Linux"},
      // Alpine ↔ Arch
      {"base-devel", "build-base", "Arch Linux", "Alpine Linux"},
      {"build-base", "base-devel", "Alpine Linux", "Arch Linux"},
      {"python", "python3", "Arch Linux", "Alpine Linux"},
      {"python3", "python", "Alpine Linux", "Arch Linux"},
      {"openssh", "openssh", "Arch Linux", "Alpine Linux"},
      {"openssh", "openssh", "Alpine Linux", "Arch Linux"},
      {"networkmanager", "networkmanager", "Arch Linux", "Alpine Linux"},
      {"networkmanager", "networkmanager", "Alpine Linux", "Arch Linux"},
      {"vim", "vim", "Arch Linux", "Alpine Linux"},
      {"vim", "vim", "Alpine Linux", "Arch Linux"},
      {"nginx", "nginx", "Debian", "Alpine Linux"},
      {"nginx", "nginx", "Alpine Linux", "Debian"},
      {"nginx", "nginx", "Arch Linux", "Alpine Linux"},
      {"nginx", "nginx", "Alpine Linux", "Arch Linux"},
  };

  std::vector<std::pair<std::string, std::string>> suggestions;
  for (const auto &pkg : packages) {
    for (const auto &m : guesses) {
      if (pkg == m.from && m.src == source_distro && m.tgt == target_distro) {
        suggestions.push_back({pkg, m.to});
        break;
      }
    }
  }
  return suggestions;
}

// Show auto-guess results to the user for review
static void show_auto_guess_dialog(const std::vector<std::string> &packages,
                                   const std::string &source_distro,
                                   const std::string &target_distro) {
  auto suggestions =
      auto_guess_packages(packages, source_distro, target_distro);
  if (suggestions.empty())
    return;

  std::string body = "ULI auto-detected possible package translations\n"
                     "from " +
                     source_distro + " → " + target_distro + ":\n\n";
  for (const auto &s : suggestions) {
    body += "  " + s.first + "  →  " + s.second + "\n";
  }
  body += "\n\033[1;33mThese are best-effort guesses.\033[0m\n"
          "For accurate mappings, provide a translation YAML via --dict.\n"
          "You can modify packages in the 'Additional packages' menu.";

  DialogBox::show_alert("\033[1;36m Package Translation Suggestions \033[0m",
                        body);
}
};

} // namespace repositories
} // namespace contents
} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_CONTENTS_REPOSITORIES_REPO_VALIDATOR_HPP
