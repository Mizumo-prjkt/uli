#ifndef ULI_BUILTIN_DICT_HPP
#define ULI_BUILTIN_DICT_HPP

#include <string>

namespace uli {
namespace package_mgr {

// Onboard Instruction Set - Debian/Ubuntu Fallback
const std::string BUILTIN_TRANS_DEBIAN = R"(
package_manager:
  binary: "apt-get"
  update_cmd: "apt-get update"
  install_cmd: "DEBIAN_FRONTEND=noninteractive apt-get install -y"
  remove_cmd: "DEBIAN_FRONTEND=noninteractive apt-get remove -y"
  clean_cmd: "apt-get autoremove -y && apt-get clean"

package_map:
  base_system: "base-files base-passwd bash bsdutils coreutils dash debianutils diffutils findutils grep gzip hostname login ncurses-base ncurses-bin perl-base sed tar util-linux"
  kernel: "linux-image-amd64"
  base_devel: "build-essential pkg-config libtool autoconf automake"
  kernel_headers: "linux-headers-amd64"
  python: "python3 python3-pip python3-venv"
  ssh: "openssh-server"
  network_manager: "network-manager"
  firewall: "ufw"
  git: "git"
  zsh: "zsh"
  sudo: "sudo"
  curl: "curl"
  wget: "wget"
  web_server: "nginx"
  db_server: "mariadb-server"
  php: "php php-cli php-fpm php-mysql"
  ntp_systemd: "systemd-timesyncd"
  ntp_chrony: "chrony"
  plymouth: "plymouth plymouth-themes"
  bootloader: "grub-efi-amd64 os-prober"
  firefox: "firefox-esr"
  x11: "xorg"
  gnome_minimal: "gnome-core"
  kde_minimal: "kde-plasma-desktop"
)";

// Onboard Instruction Set - Arch Linux Fallback
const std::string BUILTIN_TRANS_ARCH = R"(
package_manager:
  binary: "pacman"
  update_cmd: "pacman -Sy"
  install_cmd: "pacman -S --noconfirm"
  remove_cmd: "pacman -Rs --noconfirm"
  clean_cmd: "pacman -Sc --noconfirm"

package_map:
  base_system: "base linux linux-firmware"
  kernel: "linux"
  base_devel: "base-devel"
  kernel_headers: "linux-headers"
  python: "python python-pip"
  ssh: "openssh"
  network_manager: "networkmanager"
  firewall: "ufw"
  git: "git"
  zsh: "zsh"
  sudo: "sudo"
  curl: "curl"
  wget: "wget"
  web_server: "nginx"
  db_server: "mariadb"
  php: "php php-fpm"
  ntp_systemd: "systemd"
  ntp_chrony: "chrony"
  plymouth: "plymouth"
  bootloader: "grub efibootmgr os-prober"
  firefox: "firefox"
  x11: "xorg-server xorg-xinit"
  gnome_minimal: "gnome-shell"
  kde_minimal: "plasma-desktop"
)";

// Onboard Instruction Set - Alpine Linux Fallback
const std::string BUILTIN_TRANS_ALPINE = R"(
package_manager:
  binary: "apk"
  update_cmd: "apk update"
  install_cmd: "apk add"
  remove_cmd: "apk del"
  clean_cmd: "apk cache clean"

package_map:
  base_system: "alpine-base alpine-conf alpine-keys alpine-mirrors"
  kernel: "linux-lts linux-firmware-none"
  base_devel: "build-base"
  kernel_headers: "linux-headers"
  python: "python3 py3-pip"
  ssh: "openssh"
  network_manager: "networkmanager"
  firewall: "ufw"
  git: "git"
  zsh: "zsh"
  sudo: "sudo"
  curl: "curl"
  wget: "wget"
  web_server: "nginx"
  db_server: "mariadb mariadb-client"
  php: "php83 php83-common php83-fpm"
  ntp_systemd: "chrony" # Alpine usually defaults to chrony or openntpd
  ntp_chrony: "chrony"
  plymouth: "plymouth"
  bootloader: "grub grub-efi efibootmgr"
  firefox: "firefox"
  x11: "xorg-server"
  gnome_minimal: "gnome-desktop"
  kde_minimal: "plasma-desktop"
)";

} // namespace package_mgr
} // namespace uli

#endif // ULI_BUILTIN_DICT_HPP
