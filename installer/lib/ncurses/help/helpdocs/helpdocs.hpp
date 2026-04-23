#pragma once
// helpdocs.hpp - Help documentation content for HarukaInstaller TUI
// DOS-style popup help text, context-sensitive per page.

#include <string>
#include <vector>
#include <map>

namespace HelpDocs {

struct HelpEntry {
    std::string title;
    std::string body;
};

// General navigation help
inline HelpEntry general() {
    return {
        "HarukaInstaller - Quick Help",
        "Welcome to HarukaInstaller!\n"
        "\n"
        "NAVIGATION\n"
        "  UP/DOWN       Navigate menu items or lists\n"
        "  LEFT          Return to sidebar from content\n"
        "  RIGHT/ENTER   Enter content panel / confirm\n"
        "  TAB           Cycle through page sections\n"
        "  ESC           Return to sidebar\n"
        "  q             Quit installer (from sidebar)\n"
        "  F1            Show this help window\n"
        "  Shift+F1      Search wiki guide / index\n"
        "\n"
        "SIDEBAR\n"
        "  Use UP/DOWN to select a menu item.\n"
        "  Press RIGHT or ENTER to view/edit that page.\n"
        "\n"
        "CONTENT PANEL\n"
        "  Each page has its own controls.\n"
        "  Use TAB to cycle between sections within\n"
        "  a page. Use SPACE to toggle checkboxes.\n"
        "  Press ESC or LEFT to return to sidebar.\n"
        "\n"
        "ACTIONS\n"
        "  Save Config   Save your selections\n"
        "  INSTALL!      Begin installation\n"
        "  Abort         Exit without installing\n"
        "\n"
        "Press any key to close this help window."
    };
}

// Page-specific help
inline std::map<std::string, HelpEntry> page_help() {
    return {
        {"Installer Language", {
            "Language Selection",
            "Select the language for the installer UI.\n"
            "\n"
            "  UP/DOWN   Navigate the language list\n"
            "  ENTER     Confirm your selection\n"
            "  *         Marks the currently selected language\n"
            "\n"
            "This sets the installer interface language.\n"
            "System locale is configured separately in\n"
            "the 'Keyboard and Locale' section.\n"
            "\n"
            "Press any key to close."
        }},
        {"Mirror Configuration", {
            "Mirror Repository Configuration",
            "Configure pacman mirror servers.\n"
            "\n"
            "  UP/DOWN   Navigate mirrors\n"
            "  ENTER     Toggle mirror on/off\n"
            "  [*]       Enabled mirror\n"
            "  [ ]       Disabled mirror\n"
            "\n"
            "Enabled mirrors will be written to\n"
            "/etc/pacman.d/mirrorlist during install.\n"
            "\n"
            "Press any key to close."
        }},
        {"Storage Device Configuration", {
            "Storage / Disk Configuration",
            "Configure disk partitions for installation.\n"
            "\n"
            "  LEFT/RIGHT  Switch between disks\n"
            "  TAB         Cycle: disk > partitions > actions\n"
            "  UP/DOWN     Navigate partitions or actions\n"
            "\n"
            "BAR GRAPH\n"
            "  Shows partition allocation with colors:\n"
            "  Green=ext4  Yellow=fat32  Cyan=btrfs\n"
            "  Red=swap    Magenta=xfs   Black=free\n"
            "\n"
            "ACTIONS\n"
            "  Create a New Table  Create GPT/MBR table\n"
            "  Create Partition    Add a new partition\n"
            "  Delete Partition    Remove selected partition\n"
            "\n"
            "Press any key to close."
        }},
        {"Network", {
            "Network Configuration",
            "Configure network connectivity.\n"
            "\n"
            "  TAB         Cycle: interfaces > hostname\n"
            "              > actions > WiFi list\n"
            "  ENTER       Select/edit/connect\n"
            "\n"
            "INTERFACES\n"
            "  Select an interface, then use Actions:\n"
            "  - Connect/Disconnect toggle\n"
            "  - Configure IP (DHCP or static)\n"
            "  - Scan WiFi (wireless interfaces only)\n"
            "\n"
            "WIFI\n"
            "  Select a WiFi network and press ENTER\n"
            "  to connect. Signal strength shown as bars.\n"
            "\n"
            "Press any key to close."
        }},
        {"Kernels", {
            "Kernel Selection",
            "Select which kernels to install.\n"
            "\n"
            "  UP/DOWN   Navigate kernel list\n"
            "  SPACE     Toggle kernel on/off\n"
            "  [x]       Selected for install\n"
            "  [ ]       Not selected\n"
            "\n"
            "At least one kernel must be selected.\n"
            "You can install multiple kernels.\n"
            "\n"
            "Available kernels:\n"
            "  linux          Default kernel\n"
            "  linux-lts      Long-term support\n"
            "  linux-zen      Desktop-optimized\n"
            "  linux-hardened Security-focused\n"
            "\n"
            "Press any key to close."
        }},
        {"Time and Date", {
            "Time and Date Configuration",
            "Configure timezone and time sync.\n"
            "\n"
            "  TAB       Cycle: NTP > Region > City > hwclock\n"
            "  SPACE     Toggle NTP / hwclock\n"
            "  UP/DOWN   Navigate region/city lists\n"
            "\n"
            "NTP\n"
            "  Network Time Protocol keeps your clock\n"
            "  synced automatically. Recommended.\n"
            "\n"
            "hwclock --systohc\n"
            "  Syncs hardware clock to system time.\n"
            "  Enable this for dual-boot systems.\n"
            "\n"
            "Press any key to close."
        }},
    };
}

// Wiki/Guide entries for help search
inline std::map<std::string, HelpEntry> wiki_entries() {
    return {
        {"Partitioning Guide", {
            "Guide: Disk Partitioning",
            "When setting up Linux, you typically need:\n"
            "1. EFI Partition (300-512MB, FAT32)\n"
            "2. Swap Partition (depends on RAM size)\n"
            "3. Root Partition (rest of disk, ext4/btrfs)\n"
            "\n"
            "If using UEFI, the EFI partition is MANDATORY."
        }},
        {"Network Troubleshooting", {
            "Guide: Network Issues",
            "If network fails:\n"
            "- Ensure Ethernet cable is plugged in\n"
            "- For WiFi, check if the driver is loaded\n"
            "- Try pinging 8.8.8.8 to check connectivity"
        }},
        {"Bootloader Info", {
            "Guide: Bootloaders",
            "GRUB: Works on almost everything, BIOS and UEFI.\n"
            "systemd-boot: Modern, simple, UEFI-only.\n"
            "\n"
            "Pick GRUB if you are unsure."
        }},
        {"Mirror Speed", {
            "Guide: Pacman Mirrors",
            "Choose mirrors geographically close to you\n"
            "for the best download speeds.\n"
            "You can enable multiple mirrors."
        }},
        {"Root vs User", {
            "Guide: Account Security",
            "ROOT: System administrator. Full power.\n"
            "USER: Regular account for daily use.\n"
            "\n"
            "Never use the Root account for browsing\n"
            "or daily tasks."
        }},
        {"ZRAM Benefits", {
            "Guide: ZRAM and Swap",
            "ZRAM creates a compressed swap in RAM.\n"
            "It is faster than disk swap and can\n"
            "effectively increase your memory capacity."
        }},
    };
}

} // namespace HelpDocs
