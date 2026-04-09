// Universal Linux Installer
// License: MIT

#include <iostream>
#include <vector>
#include <string>
#include "lib/package_mgr/dpkg_apt/dpkg_apt.hpp"
#include "lib/package_mgr/alps/alps_mgr.hpp"
#include "lib/package_mgr/apk/apk_mgr.hpp"
#include "lib/documentation/helpguide.hpp"
#include "lib/package_mgr/repofetch.hpp"
#include "lib/package_mgr/builtin_dict.hpp"
#include "lib/documentation/extrahelp.hpp"
#include "lib/arglinter.hpp"
#include "lib/distribution_checks.hpp"
#include "lib/distro_version.hpp"
#include "lib/bootarg_check/bootargchecker.hpp"
#include "lib/chroot_hook/authorization.hpp"
#include "lib/runtime_check.hpp"
#include "lib/partitioner/diskcheck.hpp"
#include "lib/partitioner/partition_translator.hpp"
#include "lib/partitioner/partdisk/partdisk.hpp"
#include "lib/runtime/ui.hpp"
#include "lib/runtime/test_simulation.hpp"
#include "lib/runtime/term_capable_check.hpp"
#include "lib/runtime/sudden_abort.hpp"
#include "lib/config/yaml_linter.hpp"
#include <unistd.h>
#include <memory>
#include <cstdlib>
#include "uli_version.hpp"

using namespace uli::package_mgr;

void handle_terminal_resize(int) {
    // Dummy handler emitting EINTR to interrupt read() causing UI logic to re-render
}

int main(int argc, char* argv[]) {
    uli::runtime::SuddenAbort::register_handlers();
#ifdef ULI_DEBUG_MODE
    std::vector<std::string> valid_flags = {"--help", "-h", "--extra-help", "--profile", "--dict", "--force-small-disk", "--test-memleak", "--debug-mode", "--no-masking", "--dist", "--disk", "--disk-type", "--hardware", "--lintcheck", "--disable-builtin-dict", "--unattended", "--fetch-repos", "--version", "-v"};
#else
    std::vector<std::string> valid_flags = {"--help", "-h", "--extra-help", "--profile", "--dict", "--force-small-disk", "--lintcheck", "--disable-builtin-dict", "--unattended", "--version", "-v"};
#endif

    uli::checks::DistroType current_distro = uli::checks::detect_distribution();
    uli::bootarg::BootArgs bootargs = uli::bootarg::parse_boot_args();
    std::string cli_profile_path = "";
    bool run_lint = false;
    bool disable_builtin_dict = false;

    if (!bootargs.distro_override.empty()) {
        if (bootargs.distro_override == "debian" || bootargs.distro_override == "ubuntu") {
            current_distro = uli::checks::DistroType::DEBIAN;
        } else if (bootargs.distro_override == "arch") {
            current_distro = uli::checks::DistroType::ARCH;
        } else if (bootargs.distro_override == "alpine") {
            current_distro = uli::checks::DistroType::ALPINE;
        }
    }

    // Basic argument parsing
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            uli::documentation::print_help_guide();
            return 0;
        } else if (arg == "--extra-help") {
            uli::documentation::print_extra_help();
            return 0;
        } else if (arg == "--version" || arg == "-v") {
            std::cout << "ULI Installer version " << uli::PROJECT_VERSION << " (" << uli::BUILD_TYPE << ")" << std::endl;
            return 0;
#ifdef ULI_DEBUG_MODE
        } else if (arg == "--test-memleak") {
            std::cout << "[MEMLEAK] Starting memory leak diagnostic loop. PID: " << getpid() << std::endl;
            
            std::unique_ptr<PackageManagerInterface> pm;
            if (current_distro == uli::checks::DistroType::DEBIAN) {
                pm = std::make_unique<DpkgAptManager>();
            } else if (current_distro == uli::checks::DistroType::ARCH) {
                pm = std::make_unique<uli::package_mgr::alps::AlpsManager>();
            } else {
                std::cerr << "[MEMLEAK] Cannot test on unknown distribution." << std::endl;
                return 1;
            }
            
            // Adjust path depending on where it's run
            std::string trans_yaml_path = "../tests/yaml/yaml_tests/translator/trans.yaml";
            if (!bootargs.translation_yaml.empty()) {
                trans_yaml_path = bootargs.translation_yaml;
            }
            
            if (!pm->load_translation(trans_yaml_path)) {
                if (bootargs.translation_yaml.empty()) {
                    trans_yaml_path = "tests/yaml/yaml_tests/translator/trans.yaml";
                    pm->load_translation(trans_yaml_path);
                } else {
                    std::cerr << "[MEMLEAK] Failed to load custom trans.yaml: " << trans_yaml_path << std::endl;
                    return 1;
                }
            }
            
            std::vector<std::string> abstract_pkgs = {"nginx", "mysql", "base_devel", "python", "custom_pkg", "aur-yay"};
            unsigned long long iter = 0;
            
            while (true) {
                // Stress test the package managers command building
                pm->build_install_command(abstract_pkgs);
                pm->build_remove_command(abstract_pkgs);
                
                iter++;
                if (iter % 1000 == 0) {
                    std::cout << "[MEMLEAK] Completed " << iter << " translation operations." << std::endl;
                }
                usleep(1000); // 1ms sleep to yield
            }
            return 0;
        } else if (arg == "--debug-mode") {
            uli::runtime::TestSimulation::enable();
        } else if (arg == "--no-masking") {
            uli::runtime::TestSimulation::get_config().no_masking = true;
        } else if (arg == "--dist" && i + 1 < argc) {
            std::string raw_dist = argv[++i];
            auto parsed = uli::checks::DistroVersion::parse_dist_flag(raw_dist);
            uli::runtime::TestSimulation::get_config().distribution = parsed.first;
            uli::runtime::TestSimulation::get_config().distro_version = parsed.second;
        } else if (arg == "--disk" && i + 1 < argc) {
            uli::runtime::TestSimulation::get_config().disk_size = argv[++i];
        } else if (arg == "--disk-type" && i + 1 < argc) {
            uli::runtime::TestSimulation::get_config().disk_type = argv[++i];
        } else if (arg == "--hardware" && i + 1 < argc) {
            uli::runtime::TestSimulation::get_config().hardware = argv[++i];
#endif
        } else if (arg == "--profile" && i + 1 < argc) {
            cli_profile_path = argv[++i];
        } else if (arg == "--dict" && i + 1 < argc) {
            bootargs.translation_yaml = argv[++i];
        } else if (arg == "--lintcheck") {
            run_lint = true;
        } else if (arg == "--disable-builtin-dict") {
            disable_builtin_dict = true;
        } else if (arg == "--force-small-disk") {
            bootargs.force_small_disk = true;
        } else if (arg == "--unattended") {
            bootargs.unattended = true;
#ifdef ULI_DEBUG_MODE
        } else if (arg == "--fetch-repos") {
            uli::runtime::TestSimulation::get_config().fetch_repos = true;
#endif
        } else if (arg.length() >= 2 && arg.substr(0, 2) == "--") {
            bool known = false;
            for (const auto& f : valid_flags) {
                if (arg == f) {
                    known = true;
                    break;
                }
            }
            if (!known) {
                std::cerr << "Unknown flag: " << arg << "\n";
                std::string suggestion = uli::arglinter::suggest_flag(arg, valid_flags);
                if (!suggestion.empty()) {
                    std::cerr << "Did you mean '" << suggestion << "'?\n";
                }
                return 1;
            }
        }
    }

    // ──────────────────────────────────────────────────
    // YAML Linter Execution Segment
    // ──────────────────────────────────────────────────
    if (run_lint) {
        std::string final_profile = cli_profile_path;
        if (final_profile.empty()) final_profile = bootargs.config_file;

        if (final_profile.empty()) {
            std::cerr << "[ERROR] --lintcheck requires a profile. Use --profile <path> or uli.yaml" << std::endl;
            return 1;
        }

        std::string final_dict = bootargs.translation_yaml;
        bool dict_provided_explicitly = !final_dict.empty();
        bool using_builtin = false;

        // Check standard test path if bootarg is empty
        if (final_dict.empty()) {
            if (std::filesystem::exists("../tests/yaml/yaml_tests/translator/trans.yaml")) 
                final_dict = "../tests/yaml/yaml_tests/translator/trans.yaml";
            else if (std::filesystem::exists("tests/yaml/yaml_tests/translator/trans.yaml"))
                final_dict = "tests/yaml/yaml_tests/translator/trans.yaml";
            else if (!disable_builtin_dict) {
                using_builtin = true;
                if (current_distro == uli::checks::DistroType::ARCH) final_dict = "BUILTIN_TRANS_ARCH";
                else if (current_distro == uli::checks::DistroType::ALPINE) final_dict = "BUILTIN_TRANS_ALPINE";
                else final_dict = "BUILTIN_TRANS_DEBIAN";
            }
        }

        auto issues = uli::runtime::config::YamlLinter::lint(final_profile, final_dict, dict_provided_explicitly, using_builtin);
        uli::runtime::config::YamlLinter::print_report(issues);

        if (uli::runtime::config::YamlLinter::has_fatal(issues)) return 1;
        return 0;
    }

    int detected_debian_version = 0;
#ifdef ULI_DEBUG_MODE
    if (uli::runtime::TestSimulation::get_config().fetch_repos) {
        std::string distro = uli::runtime::TestSimulation::get_config().distribution;
        if (distro.empty()) {
            if (current_distro == uli::checks::DistroType::ARCH) distro = "arch";
            else if (current_distro == uli::checks::DistroType::ALPINE) distro = "alpine";
            else distro = "debian";
        }

        std::cout << "[DEBUG] Starting repo fetch for distribution: " << distro << std::endl;
        auto fetcher = uli::debug::FetcherFactory::create(distro);
        if (fetcher) {
            std::vector<std::string> urls;
            if (distro == "arch") {
                urls = {
                    "https://mirror.osbeck.com/archlinux/core/os/x86_64/core.db",
                    "https://mirror.osbeck.com/archlinux/extra/os/x86_64/extra.db"
                };
            } else if (distro == "alpine") {
                urls = {
                    "https://dl-cdn.alpinelinux.org/alpine/v3.19/main/x86_64/APKINDEX.tar.gz",
                    "https://dl-cdn.alpinelinux.org/alpine/v3.19/community/x86_64/APKINDEX.tar.gz"
                };
            } else {
                urls = {
                    "http://ftp.debian.org/debian/dists/trixie/main/binary-amd64/Packages.xz",
                    "http://security.debian.org/debian-security/dists/trixie-security/main/binary-amd64/Packages.xz",
                    "http://ftp.debian.org/debian/dists/trixie-updates/main/binary-amd64/Packages.xz"
                };
            }

            for (const auto& url : urls) {
                if (fetcher->fetch_and_parse(url)) {
                    std::cout << "[DEBUG] Fetch successful: " << url << std::endl;
                } else {
                    std::cerr << "[ERROR] Repository fetch failed: " << url << std::endl;
                }
            }
            
            auto pkgs = fetcher->get_packages();
            std::cout << "[DEBUG] Total packages found across all repos: " << pkgs.size() << std::endl;
            if (!pkgs.empty()) {
                std::cout << "[DEBUG] Sample package: " << pkgs[0].name << " (" << pkgs[0].version << ")" << std::endl;
            }
        } else {
            std::cerr << "[ERROR] No fetcher available for distro: " << distro << std::endl;
        }
        return 0; // Exit after debug fetch
    }
#endif

    // Auto-detect Debian version from live system if not explicitly set
    if (detected_debian_version == 0 && current_distro == uli::checks::DistroType::DEBIAN) {
        detected_debian_version = uli::checks::DistroVersion::detect_major_version();
    }

    std::cout << "Universal Linux Installer (ULI)" << std::endl;
    // Determine the relevant PM
    std::unique_ptr<PackageManagerInterface> pm;
    std::string distro_name = "Unknown";
    
    if (current_distro == uli::checks::DistroType::DEBIAN) {
        pm = std::make_unique<DpkgAptManager>();
        distro_name = "Debian"; // Default fallback
        if (!bootargs.distro_override.empty()) {
            if (bootargs.distro_override == "ubuntu") distro_name = "Ubuntu";
        } else if (uli::runtime::TestSimulation::is_enabled() && !uli::runtime::TestSimulation::get_config().distribution.empty()) {
            if (uli::runtime::TestSimulation::get_config().distribution == "ubuntu") distro_name = "Ubuntu";
        } else {
            // Validate system config files resolving ubuntu forks dynamically
            std::ifstream file("/etc/os-release");
            std::string line;
            if (file.is_open()) {
                while (std::getline(file, line)) {
                    if (line.find("ID=ubuntu") == 0 || line.find("ID=\"ubuntu\"") == 0) {
                        distro_name = "Ubuntu";
                        break;
                    }
                }
            }
        }
    } else if (current_distro == uli::checks::DistroType::ARCH) {
        pm = std::make_unique<uli::package_mgr::alps::AlpsManager>();
        distro_name = "Arch Linux";
    } else if (current_distro == uli::checks::DistroType::ALPINE) {
        pm = std::make_unique<uli::package_mgr::apk::ApkManager>();
        distro_name = "Alpine Linux";
    } else {
        std::cerr << "Error: Unsupported or unknown Linux distribution." << std::endl;
        return 1;
    }

    std::cout << "Detected Active Distribution: " << distro_name;
    if (!bootargs.distro_override.empty()) {
        std::cout << " (Overridden by BootArg)";
    }
    std::cout << std::endl;

    if (uli::env::RuntimeEnv::is_live_environment()) {
        std::cout << "[INFO] Detected Execution in Live CD/USB Environment." << std::endl;
    } else {
        std::cout << "[INFO] Standard Host Operating System Execution." << std::endl;
    }
    
    // ──────────────────────────────────────────────────
    // Translation Loading logic
    // ──────────────────────────────────────────────────
    bool loaded = false;
    std::string final_dict_path = bootargs.translation_yaml;

    // 1. Try explicit --dict path
    if (!final_dict_path.empty()) {
        if (pm->load_translation(final_dict_path)) {
            loaded = true;
        } else {
            std::cerr << "[ERROR] Could not load specified dictionary: " << final_dict_path << std::endl;
            return 1;
        }
    }

    // 2. Try standard search paths (but only if they match our distro)
    if (!loaded) {
        std::vector<std::string> search_paths = {
            "trans.yaml",
            "uli_profile.yaml",
            "tests/yaml/yaml_tests/translator/trans.yaml",
            "../tests/yaml/yaml_tests/translator/trans.yaml"
        };
        for (const auto& path : search_paths) {
            if (std::filesystem::exists(path)) {
                // Peek at the file to check distro_id
                try {
                    YAML::Node node = YAML::LoadFile(path);
                    std::string yaml_distro = "";
                    if (node["distro_id"]) yaml_distro = node["distro_id"].as<std::string>();
                    
                    bool match = false;
                    if (current_distro == uli::checks::DistroType::ARCH && yaml_distro == "arch") match = true;
                    else if (current_distro == uli::checks::DistroType::DEBIAN && yaml_distro == "debian") match = true;
                    else if (current_distro == uli::checks::DistroType::ALPINE && yaml_distro == "alpine") match = true;

                    if (match && pm->load_translation(path)) {
                        std::cout << "[INFO] Found matching external dictionary at: " << path << std::endl;
                        loaded = true;
                        break;
                    }
                } catch (...) {}
            }
        }
    }

    // 3. Fallback to builtins
    if (!loaded && !disable_builtin_dict) {
        std::cout << "[INFO] Using embedded multi-distro fallback for " << distro_name << std::endl;
        std::string builtin_str;
        if (current_distro == uli::checks::DistroType::ARCH) builtin_str = uli::package_mgr::BUILTIN_TRANS_ARCH;
        else if (current_distro == uli::checks::DistroType::ALPINE) builtin_str = uli::package_mgr::BUILTIN_TRANS_ALPINE;
        else builtin_str = uli::package_mgr::BUILTIN_TRANS_DEBIAN;
        
        if (pm->load_translation_from_string(builtin_str)) {
            loaded = true;
        }
    }

    if (!loaded) {
        std::cerr << "[FATAL] No translation dictionary loaded. Installation cannot proceed." << std::endl;
        return 1;
    }

    // Verification PKGs
    std::vector<std::string> abstract_pkgs = {"base_system", "kernel", "base_devel", "python", "git", "web_server"};
    
    // Check if we are in bootstrap mode (installing to /mnt)
    if (uli::env::RuntimeEnv::is_live_environment()) {
        setenv("ULI_BOOTSTRAP", "1", 1);
    }

    std::string command = pm->build_install_command(abstract_pkgs);
    std::cout << "\nGenerated Install Command (" << (std::getenv("ULI_BOOTSTRAP") ? "Bootstrap" : "Host") << "):" << std::endl;
    std::cout << "  " << command << std::endl;

    if (!bootargs.unattended) {
        std::cout << "\n[INFO] Interactive Mode sequence initiated...\n";
        
        // Setup VT buffering
        uli::runtime::TermCapableCheck::register_resize_handler(handle_terminal_resize);
        std::atexit(uli::runtime::TermCapableCheck::exit_alternate_screen);
        uli::runtime::TermCapableCheck::enter_alternate_screen();
        
        uli::runtime::MenuState preloaded_state;
        std::string final_profile = cli_profile_path;
        if (final_profile.empty()) final_profile = bootargs.config_file;

        if (!final_profile.empty()) {
            std::cout << "[INFO] Loading configuration profile: " << final_profile << std::endl;
            if (!uli::runtime::config::ConfigLoader::load_yaml_to_menu_state(final_profile, preloaded_state)) {
                std::cerr << "[ERROR] Configuration loading failed. Aborting to prevent unpredictable installation." << std::endl;
                uli::runtime::TermCapableCheck::exit_alternate_screen();
                return 1;
            }
        }

        uli::runtime::UIManager::start_ui(distro_name, detected_debian_version, preloaded_state);
    } else {
        std::cout << "\n[INFO] Unattended Mode sequence initiated. Processing config instructions automatically.\n";
        // Run Disk Partition Analysis
        std::cout << "\n--- Logical Partition Evaluator ---" << std::endl;
        // Mock user preference from config or yaml logic: e.g., "" means 'auto select' 
        std::string user_disk_preference = "";
        std::string final_install_disk = uli::partitioner::DiskCheck::select_target_disk(user_disk_preference);
        
        if (final_install_disk.empty()) {
             std::cout << "[INFO] Installation halted. Disk check evaluation declined further modifications.\n";
        } else {
             std::cout << "[INFO] Final Verified Installation Block: " << final_install_disk << std::endl;
             // uli::partitioner::partdisk::PartDisk::prepare_efi_layout(final_install_disk);
        }
    }

    std::string chroot_cmd = "chroot";
    if (distro_name == "Arch Linux") {
        chroot_cmd = "arch-chroot";
    }

    // Authorization Test
    if (!uli::hook::ChrootHook::is_root()) {
        std::cout << "\n[INFO] Normal user execution detected. Chroot execution capabilities are disabled.\n";
        std::cout << "To test chroot, please run uli_installer as root (sudo).\n";
    } else {
        std::cout << "\n[INFO] Root privileges detected. Assuming /mnt is bootstrap target. Using command: " << chroot_cmd << "\n";
        // Uncomment to execute:
        // uli::hook::ChrootHook::execute_in_chroot("/mnt", command, chroot_cmd);
    }

    return 0;
}
