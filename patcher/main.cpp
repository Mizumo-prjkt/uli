// Universal Linux Installer — ISO Patcher
// Injects uli_installer + profile.yaml into Linux ISO images
// and modifies boot menus to add a "Install with ULI" entry.
// License: MIT

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <unistd.h>

#include "lib/iso_backend.hpp"
#include "lib/boot_detector.hpp"
#include "uli_version.hpp"
#include "lib/boot_injector.hpp"
#include "lib/autostart_gen.hpp"

namespace fs = std::filesystem;

struct PatcherArgs {
    std::string iso_path;
    std::string installer_path;
    std::string profile_path;
    std::string output_path;
    bool help = false;
    bool version = false;
    bool dry_run = false;
};

void print_help() {
    std::cout << R"(
ULI Patcher — ISO Injection & Boot Menu Modification Tool

Usage:
  uli_patcher --iso <input.iso> --installer <uli_installer> --profile <profile.yaml> --output <output.iso>

Required Arguments:
  --iso         Path to the source Linux ISO image
  --installer   Path to the uli_installer binary to inject
  --profile     Path to the YAML profile to inject

Optional Arguments:
  --output      Path for the patched output ISO (default: <input>_uli.iso)
  --dry-run     Analyze the ISO without making changes
  --help, -h    Show this help message
  --version, -v Show the project version

Example:
  uli_patcher --iso archlinux-2026.04.01-x86_64.iso \
              --installer build/uli_installer \
              --profile build/compositor/uli_profile.yaml \
              --output archlinux-uli.iso

What it does:
  1. Opens the source ISO
  2. Creates /uli/ directory inside the ISO
  3. Injects uli_installer binary as /uli/uli_installer
  4. Injects YAML profile as /uli/profile.yaml
  5. Generates an autostart hook script at /uli/autostart.sh
  6. Detects boot configurations (GRUB, Isolinux/Syslinux)
  7. Appends an "Install with ULI Installer" boot menu entry
  8. Writes the patched ISO
)" << std::endl;
}

PatcherArgs parse_args(int argc, char* argv[]) {
    PatcherArgs args;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            args.help = true;
        } else if (arg == "--version" || arg == "-v") {
            args.version = true;
        } else if (arg == "--dry-run") {
            args.dry_run = true;
        } else if (arg == "--iso" && i + 1 < argc) {
            args.iso_path = argv[++i];
        } else if (arg == "--installer" && i + 1 < argc) {
            args.installer_path = argv[++i];
        } else if (arg == "--profile" && i + 1 < argc) {
            args.profile_path = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            args.output_path = argv[++i];
        } else {
            // Ignore if the argument is just the binary name (common in some wrapper calls)
            if (arg != "uli_patcher" && arg != "./uli_patcher") {
                std::cerr << "[Patcher] Unknown argument: " << arg << std::endl;
            }
        }
    }

    return args;
}

int main(int argc, char* argv[]) {
    PatcherArgs args = parse_args(argc, argv);

    if (args.help || argc < 2) {
        print_help();
        return 0;
    }

    if (args.version) {
        std::cout << "ULI Patcher version " << uli::PROJECT_VERSION << " (" << uli::BUILD_TYPE << ")" << std::endl;
        return 0;
    }

    // Validate required arguments
    if (args.iso_path.empty()) {
        std::cerr << "[Patcher] Error: --iso is required." << std::endl;
        return 1;
    }
    if (args.installer_path.empty()) {
        std::cerr << "[Patcher] Error: --installer is required." << std::endl;
        return 1;
    }
    if (args.profile_path.empty()) {
        std::cerr << "[Patcher] Error: --profile is required." << std::endl;
        return 1;
    }

    // Resolve to absolute paths
    args.iso_path = fs::absolute(args.iso_path).string();
    args.installer_path = fs::absolute(args.installer_path).string();
    args.profile_path = fs::absolute(args.profile_path).string();

    // Validate files exist
    if (!fs::exists(args.iso_path)) {
        std::cerr << "[Patcher] Error: ISO not found: " << args.iso_path << std::endl;
        return 1;
    }
    if (!fs::exists(args.installer_path)) {
        std::cerr << "[Patcher] Error: Installer binary not found: " << args.installer_path << std::endl;
        return 1;
    }
    if (!fs::exists(args.profile_path)) {
        std::cerr << "[Patcher] Error: Profile not found: " << args.profile_path << std::endl;
        return 1;
    }

    // Default output path
    if (args.output_path.empty()) {
        std::string stem = fs::path(args.iso_path).stem().string();
        std::string dir = fs::path(args.iso_path).parent_path().string();
        args.output_path = dir + "/" + stem + "_uli.iso";
    }
    args.output_path = fs::absolute(args.output_path).string();

    std::cout << "================================================" << std::endl;
    std::cout << "  ULI Patcher — ISO Injection Tool" << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << "  Source ISO:  " << args.iso_path << std::endl;
    std::cout << "  Installer:   " << args.installer_path << std::endl;
    std::cout << "  Profile:     " << args.profile_path << std::endl;
    std::cout << "  Output ISO:  " << args.output_path << std::endl;
    std::cout << "  Dry Run:     " << (args.dry_run ? "Yes" : "No") << std::endl;
    std::cout << "================================================" << std::endl;

    // Create ISO backend
    auto backend = uli::patcher::create_iso_backend();
    if (!backend) {
        std::cerr << "[Patcher] Fatal: No usable ISO backend available." << std::endl;
        std::cerr << "[Patcher] Install libisoburn-dev or ensure xorriso is in PATH." << std::endl;
        return 1;
    }
    std::cout << "[Patcher] Backend: " << backend->name() << std::endl;

    // Open the ISO
    std::cout << "\n[Step 1/6] Opening source ISO..." << std::endl;
    auto open_res = backend->open(args.iso_path);
    if (!open_res.success) {
        std::cerr << "[Patcher] Failed to open ISO: " << open_res.error << std::endl;
        return 1;
    }

    // Detect boot configurations
    std::cout << "\n[Step 2/6] Detecting boot configurations..." << std::endl;
    auto boot_configs = uli::patcher::BootDetector::detect(*backend);

    if (args.dry_run) {
        std::cout << "\n[Dry Run] Found " << boot_configs.size() << " boot config(s)." << std::endl;
        for (const auto& cfg : boot_configs) {
            std::cout << "  - " << cfg.iso_path << " (" << cfg.type_str() << ")" << std::endl;
        }
        std::cout << "[Dry Run] No changes made." << std::endl;
        backend->close();
        return 0;
    }

    // Create /uli/ directory and inject files
    std::cout << "\n[Step 3/6] Injecting ULI files..." << std::endl;
    backend->mkdir("/uli");

    auto inj_installer = backend->inject_file(args.installer_path, "/uli/uli_installer");
    if (!inj_installer.success) {
        std::cerr << "[Patcher] Failed to inject installer: " << inj_installer.error << std::endl;
        backend->close();
        return 1;
    }
    std::cout << "  [+] /uli/uli_installer" << std::endl;

    auto inj_profile = backend->inject_file(args.profile_path, "/uli/profile.yaml");
    if (!inj_profile.success) {
        std::cerr << "[Patcher] Failed to inject profile: " << inj_profile.error << std::endl;
        backend->close();
        return 1;
    }
    std::cout << "  [+] /uli/profile.yaml" << std::endl;

    // Generate and inject autostart script
    std::cout << "\n[Step 4/6] Generating autostart hook..." << std::endl;
    std::string temp_dir = fs::temp_directory_path().string() + "/uli_patcher_work_" + std::to_string(getpid());
    fs::create_directories(temp_dir);

    std::string autostart_path = uli::patcher::AutostartGen::write_to_temp(temp_dir);
    if (autostart_path.empty()) {
        std::cerr << "[Patcher] Failed to generate autostart script." << std::endl;
        backend->close();
        return 1;
    }

    auto inj_autostart = backend->inject_file(autostart_path, "/uli/autostart.sh");
    if (!inj_autostart.success) {
        std::cerr << "[Patcher] Failed to inject autostart: " << inj_autostart.error << std::endl;
        backend->close();
        return 1;
    }
    std::cout << "  [+] /uli/autostart.sh" << std::endl;

    // Inject boot entries
    std::cout << "\n[Step 5/6] Injecting boot menu entries..." << std::endl;
    if (boot_configs.empty()) {
        std::cout << "  [!] No boot configs found — files injected but no boot entry created." << std::endl;
        std::cout << "      The installer can be run manually from /uli/autostart.sh" << std::endl;
    } else {
        int modified = uli::patcher::BootInjector::inject_all(*backend, boot_configs, temp_dir);
        std::cout << "  [+] Modified " << modified << "/" << boot_configs.size()
                  << " boot configuration(s)" << std::endl;
    }

    // Write the patched ISO
    std::cout << "\n[Step 6/6] Writing patched ISO..." << std::endl;
    auto write_res = backend->write(args.output_path);
    if (!write_res.success) {
        std::cerr << "[Patcher] Failed to write ISO: " << write_res.error << std::endl;
        backend->close();
        // Clean up temp
        fs::remove_all(temp_dir);
        return 1;
    }

    // Clean up
    backend->close();
    fs::remove_all(temp_dir);

    std::cout << "\n================================================" << std::endl;
    std::cout << "  Patching Complete!" << std::endl;
    std::cout << "  Output: " << args.output_path << std::endl;
    if (fs::exists(args.output_path)) {
        auto size = fs::file_size(args.output_path);
        std::cout << "  Size:   " << (size / 1024 / 1024) << " MiB" << std::endl;
    } else {
        std::cerr << "  Error:  Output ISO was not generated." << std::endl;
    }
    std::cout << "================================================" << std::endl;
    std::cout << "\nTo test:" << std::endl;
    std::cout << "  qemu-system-x86_64 -cdrom " << args.output_path << " -m 2G" << std::endl;

    return 0;
}
