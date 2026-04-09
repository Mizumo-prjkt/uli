#define _LARGEFILE_SOURCE
#define _FILE_OFFSET_BITS 64

#include "iso_backend.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <array>
#include <unistd.h>


#ifdef ULI_HAS_LIBISOBURN
#include <libisoburn/xorriso.h>
#endif

namespace fs = std::filesystem;

namespace uli {
namespace patcher {

// ============================================================
//  Factory
// ============================================================

std::unique_ptr<IsoBackend> create_iso_backend() {
#ifdef ULI_HAS_LIBISOBURN
    std::cout << "[Patcher] Using xorriso C API backend (libisoburn linked)" << std::endl;
    return std::make_unique<XorrisoApiBackend>();
#else
    if (XorrisoCmdBackend::is_xorriso_available()) {
        std::cout << "[Patcher] Using xorriso CLI fallback backend" << std::endl;
        return std::make_unique<XorrisoCmdBackend>();
    }
    std::cerr << "[Patcher] FATAL: Neither libisoburn nor xorriso CLI found!" << std::endl;
    return nullptr;
#endif
}

// ============================================================
//  Xorriso C API Backend
// ============================================================

#ifdef ULI_HAS_LIBISOBURN

XorrisoApiBackend::XorrisoApiBackend() {
    int ret = Xorriso_new((struct XorrisO**)&xorriso_, const_cast<char*>("uli_patcher"), 0);
    if (ret <= 0) {
        std::cerr << "[xorriso-api] Failed to create XorrisO object" << std::endl;
        return;
    }

    ret = Xorriso_startup_libraries((struct XorrisO*)xorriso_, 0);
    if (ret <= 0) {
        std::cerr << "[xorriso-api] Failed to initialize libraries" << std::endl;
        Xorriso_destroy((struct XorrisO**)&xorriso_, 0);
        xorriso_ = nullptr;
        return;
    }

    initialized_ = true;
    run_cmd("-abort_on NEVER");
    run_cmd("-osirrox on");
    run_cmd("-overwrite on");
}

XorrisoApiBackend::~XorrisoApiBackend() {
    close();
}

IsoResult XorrisoApiBackend::run_cmd(const std::string& cmd) {
    IsoResult res;
    if (!initialized_ || !xorriso_) {
        res.error = "xorriso not initialized";
        return res;
    }

    // Use execute_option with the full line. This is the most stable entry point.
    // It will internally parse the words correctly. 
    int ret = Xorriso_execute_option((struct XorrisO*)xorriso_, const_cast<char*>(cmd.c_str()), 0);
    
    if (ret <= 0) {
        res.error = "xorriso command failed: " + cmd;
        return res;
    }

    res.success = true;
    return res;
}

IsoResult XorrisoApiBackend::open(const std::string& iso_path) {
    iso_path_ = iso_path;
    // For patching/remastering, we use -indev to load the base ISO as read-only
    std::string cmd = "-indev " + iso_path;
    return run_cmd(cmd);
}

IsoResult XorrisoApiBackend::inject_file(const std::string& disk_path, const std::string& iso_path) {
    // -map maps a disk file/tree into the ISO filesystem
    std::string cmd = "-map " + disk_path + " " + iso_path;
    return run_cmd(cmd);
}

IsoResult XorrisoApiBackend::extract_file(const std::string& iso_path, const std::string& disk_path) {
    // -extract extracts from ISO to disk
    std::string cmd = "-extract " + iso_path + " " + disk_path;
    return run_cmd(cmd);
}

IsoResult XorrisoApiBackend::mkdir(const std::string& iso_path) {
    std::string cmd = "-mkdir " + iso_path;
    return run_cmd(cmd);
}

std::vector<std::string> XorrisoApiBackend::ls(const std::string& iso_path) {
    // For ls, we need to capture output — use the interpreter approach
    // For now, return empty; the boot detector uses exists() checks instead
    std::vector<std::string> results;
    return results;
}

bool XorrisoApiBackend::exists(const std::string& iso_path) {
    // -find <path> -maxdepth 0 returns 1 if found
    std::string cmd = "-find " + iso_path + " -maxdepth 0";
    int ret = Xorriso_execute_option((struct XorrisO*)xorriso_, const_cast<char*>(cmd.c_str()), 0);
    return ret == 1;
}

IsoResult XorrisoApiBackend::write(const std::string& output_path) {
    IsoResult res;
    if (!initialized_ || !xorriso_) {
        res.error = "xorriso not initialized";
        return res;
    }

    // Remove output file if it exists to allow fresh write
    if (fs::exists(output_path)) {
        fs::remove(output_path);
    }

    // 1. If we have a custom EFI image, map it into the ISO first
    if (!custom_efi_img_.empty()) {
        std::cout << "[Patcher] Mapping custom EFI image into ISO..." << std::endl;
        inject_file(custom_efi_img_, "/uli/efiboot.img");
        
        // Replay original boot settings (BIOS + UEFI) then swap UEFI only
        run_cmd("-boot_image any replay");
        run_cmd("-boot_image any partition_offset=16");
        run_cmd("-boot_image any efi_path=/uli/efiboot.img");
        run_cmd("-boot_image any next");
    }

    // 2. Set output device
    std::string out_cmd = "-outdev " + output_path;
    res = run_cmd(out_cmd);
    if (!res.success) return res;

    // 3. Commit writes the session
    res = run_cmd("-commit");
    return res;
}

IsoResult XorrisoApiBackend::extract_boot_image(const std::string& output_disk_path) {
    IsoResult res;
    if (iso_path_.empty()) {
        res.error = "No ISO path known (not opened?)";
        return res;
    }

    // 1. Get El Torito report to find the EFI partition LBA and size
    // We use standard ISO block size (2048)
    std::string report_cmd = "xorriso -indev " + iso_path_ + " -report_el_torito plain 2>&1";
    FILE* pipe = popen(report_cmd.c_str(), "r");
    if (!pipe) {
        res.error = "Failed to run xorriso report";
        return res;
    }

    char buffer[1024];
    uint64_t efi_lba = 0;
    uint32_t efi_blks = 0;
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    pclose(pipe);

    // Parse LBA and blocks from report
    // Example: "El Torito boot img :   2  UEFI  y   none  0x0000  0x00      0      621376"
    // Example: "El Torito img blks :   2  129024"
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("El Torito boot img") != std::string::npos && line.find("UEFI") != std::string::npos) {
            std::vector<std::string> words;
            std::string word;
            std::istringstream line_iss(line);
            while (line_iss >> word) words.push_back(word);
            // The LBA is usually the last word in the "boot img" line for UEFI
            if (!words.empty()) {
                try {
                    efi_lba = std::stoull(words.back());
                } catch (...) {}
            }
        } else if (line.find("El Torito img blks") != std::string::npos) {
            // Check if this line refers to image 2 (standard for UEFI in these ISOs)
            std::vector<std::string> words;
            std::string word;
            std::istringstream line_iss(line);
            while (line_iss >> word) words.push_back(word);
            // Typically: "El", "Torito", "img", "blks", ":", "2", "129024"
            for (size_t i = 0; i + 1 < words.size(); ++i) {
                if (words[i] == "2") {
                    try {
                        efi_blks = std::stoul(words[i+1]);
                        break;
                    } catch (...) {}
                }
            }
        }
    }

    if (efi_lba == 0 || efi_blks == 0) {
        res.error = "Could not identify EFI boot image LBA (found " + std::to_string(efi_lba) + 
                    ") or block count (found " + std::to_string(efi_blks) + ") from xorriso report";
        return res;
    }

    std::cout << "[Patcher] Extracted EFI image from LBA " << efi_lba << " (" << efi_blks << " blocks)" << std::endl;

    // 2. Extract using dd
    // status=none keeps it quiet, conv=sparse/notrunc etc. usually not needed for simple dump
    std::string dd_cmd = "dd if=" + iso_path_ + " skip=" + std::to_string(efi_lba) + 
                         " count=" + std::to_string(efi_blks) + " bs=2048 of=" + output_disk_path + " status=none";
    
    int ret = std::system(dd_cmd.c_str());
    if (ret != 0) {
        res.error = "dd failed to extract EFI partition (exit " + std::to_string(ret) + ")";
        return res;
    }

    res.success = true;
    return res;
}

void XorrisoApiBackend::set_efi_boot_image(const std::string& disk_path) {
    custom_efi_img_ = disk_path;
}

void XorrisoApiBackend::close() {
    if (xorriso_) {
        run_cmd("-end");
        Xorriso_destroy((struct XorrisO**)&xorriso_, 0);
        xorriso_ = nullptr;
    }
    initialized_ = false;
}

#endif // ULI_HAS_LIBISOBURN

// ============================================================
//  Xorriso CLI Fallback Backend
// ============================================================

XorrisoCmdBackend::XorrisoCmdBackend() {}

XorrisoCmdBackend::~XorrisoCmdBackend() {
    close();
}

bool XorrisoCmdBackend::is_xorriso_available() {
    int ret = std::system("which xorriso > /dev/null 2>&1");
    return ret == 0;
}

std::pair<int, std::string> XorrisoCmdBackend::exec_xorriso(const std::string& args) {
    std::string cmd = "xorriso " + args + " 2>&1";
    std::array<char, 4096> buffer;
    std::string output;
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return {-1, "Failed to execute xorriso"};
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }
    
    int status = pclose(pipe);
    int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    
    return {exit_code, output};
}

IsoResult XorrisoCmdBackend::open(const std::string& iso_path) {
    IsoResult res;
    
    if (!is_xorriso_available()) {
        res.error = "xorriso binary not found in PATH";
        return res;
    }
    
    if (!fs::exists(iso_path)) {
        res.error = "ISO file not found: " + iso_path;
        return res;
    }
    
    iso_path_ = fs::absolute(iso_path).string();
    
    // Create a temp working directory
    temp_dir_ = fs::temp_directory_path().string() + "/uli_patcher_" + std::to_string(getpid());
    fs::create_directories(temp_dir_);
    
    opened_ = true;
    res.success = true;
    std::cout << "[xorriso-cli] Opened ISO: " << iso_path_ << std::endl;
    return res;
}

IsoResult XorrisoCmdBackend::inject_file(const std::string& disk_path, const std::string& iso_path) {
    IsoResult res;
    if (!opened_) { res.error = "No ISO opened"; return res; }
    
    // For CLI mode, we batch all operations in write()
    // For now, stage the file mapping
    std::string staging = temp_dir_ + "/staged_maps.txt";
    std::ofstream staging_file(staging, std::ios::app);
    staging_file << disk_path << " " << iso_path << "\n";
    staging_file.close();
    
    res.success = true;
    return res;
}

IsoResult XorrisoCmdBackend::extract_file(const std::string& iso_path, const std::string& disk_path) {
    IsoResult res;
    if (!opened_) { res.error = "No ISO opened"; return res; }
    
    // Ensure parent dir exists
    fs::create_directories(fs::path(disk_path).parent_path());
    
    std::string args = "-indev " + iso_path_ + " -osirrox on -extract " + iso_path + " " + disk_path;
    auto [code, output] = exec_xorriso(args);
    
    if (code != 0) {
        res.error = "Extract failed (exit " + std::to_string(code) + "): " + output;
    } else {
        res.success = true;
    }
    return res;
}

IsoResult XorrisoCmdBackend::mkdir(const std::string& iso_path) {
    IsoResult res;
    // Staged for write()
    std::string staging = temp_dir_ + "/staged_mkdirs.txt";
    std::ofstream staging_file(staging, std::ios::app);
    staging_file << iso_path << "\n";
    staging_file.close();
    res.success = true;
    return res;
}

std::vector<std::string> XorrisoCmdBackend::ls(const std::string& iso_path) {
    std::vector<std::string> results;
    if (!opened_) return results;
    
    std::string args = "-indev " + iso_path_ + " -lsx " + iso_path;
    auto [code, output] = exec_xorriso(args);
    
    if (code == 0) {
        std::istringstream iss(output);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line[0] != '-') { // Skip xorriso info lines
                results.push_back(line);
            }
        }
    }
    return results;
}

bool XorrisoCmdBackend::exists(const std::string& iso_path) {
    if (!opened_) return false;
    
    // -test -e is the correct way to check existence
    std::string args = "-indev " + iso_path_ + " -test -e " + iso_path + " 2>/dev/null";
    auto [code, output] = exec_xorriso(args);
    return code == 1 || code == 0; // xorriso -test returns 1 for success usually, but exit code might vary
}

IsoResult XorrisoCmdBackend::write(const std::string& output_path) {
    IsoResult res;
    if (!opened_) { res.error = "No ISO opened"; return res; }

    std::string args = "-indev " + iso_path_;
    
    // 1. If we have a custom EFI image, map it and set as boot image
    if (!custom_efi_img_.empty()) {
        args += " -map " + custom_efi_img_ + " /uli/efiboot.img";
        args += " -boot_image any replay";
        args += " -boot_image any partition_offset=16";
        args += " -boot_image any efi_path=/uli/efiboot.img";
        args += " -boot_image any next";
    }

    // Apply staged mkdirs
    std::string mkdir_file = temp_dir_ + "/staged_mkdirs.txt";
    if (fs::exists(mkdir_file)) {
        std::ifstream f(mkdir_file);
        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty()) {
                args += " -mkdir " + line;
            }
        }
    }
    
    // Apply staged file maps
    std::string map_file = temp_dir_ + "/staged_maps.txt";
    if (fs::exists(map_file)) {
        std::ifstream f(map_file);
        std::string line;
        while (std::getline(f, line)) {
            if (!line.empty()) {
                // Parse "disk_path iso_path"
                auto space = line.find(' ');
                if (space != std::string::npos) {
                    std::string disk = line.substr(0, space);
                    std::string iso = line.substr(space + 1);
                    args += " -map " + disk + " " + iso;
                }
            }
        }
    }
    
    args += " -outdev " + output_path + " -commit -end";
    
    std::cout << "[xorriso-cli] Executing write operation..." << std::endl;
    auto [code, output] = exec_xorriso(args);
    
    if (code != 0) {
        res.error = "Write failed (exit " + std::to_string(code) + "):\n" + output;
    } else {
        res.success = true;
        std::cout << "[xorriso-cli] Patched ISO written to: " << output_path << std::endl;
    }
    return res;
}

IsoResult XorrisoCmdBackend::extract_boot_image(const std::string& output_disk_path) {
    IsoResult res;
    if (!opened_) { res.error = "No ISO opened"; return res; }

    // Use the same block-level extraction strategy as the API backend
    std::string report_args = "-indev " + iso_path_ + " -report_el_torito plain";
    auto [code, output] = exec_xorriso(report_args);
    
    if (code != 0) {
        res.error = "Failed to run xorriso report: " + output;
        return res;
    }

    uint64_t efi_lba = 0;
    uint32_t efi_blks = 0;
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find("El Torito boot img") != std::string::npos && line.find("UEFI") != std::string::npos) {
            std::vector<std::string> words;
            std::string word;
            std::istringstream line_iss(line);
            while (line_iss >> word) words.push_back(word);
            if (words.size() >= 10) efi_lba = std::stoull(words.back());
        } else if (line.find("El Torito img blks") != std::string::npos && line.find(" 2 ") != std::string::npos) {
            std::vector<std::string> words;
            std::string word;
            std::istringstream line_iss(line);
            while (line_iss >> word) words.push_back(word);
            if (words.size() >= 6) efi_blks = std::stoul(words.back());
        }
    }

    if (efi_lba == 0 || efi_blks == 0) {
        res.error = "Could not identify EFI boot image LBA or block count";
        return res;
    }

    std::cout << "[Patcher] Found EFI boot image at LBA " << efi_lba << " (" << efi_blks << " blocks)" << std::endl;

    std::string dd_cmd = "dd if=" + iso_path_ + " skip=" + std::to_string(efi_lba) + 
                         " count=" + std::to_string(efi_blks) + " bs=2048 of=" + output_disk_path + " conv=notrunc > /dev/null 2>&1";
    
    int ret = std::system(dd_cmd.c_str());
    if (ret != 0) {
        res.error = "dd extraction failed";
        return res;
    }

    res.success = true;
    return res;
}

void XorrisoCmdBackend::set_efi_boot_image(const std::string& disk_path) {
    custom_efi_img_ = disk_path;
}

void XorrisoCmdBackend::close() {
    if (!temp_dir_.empty() && fs::exists(temp_dir_)) {
        fs::remove_all(temp_dir_);
    }
    opened_ = false;
}

} // namespace patcher
} // namespace uli
