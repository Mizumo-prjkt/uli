#pragma once
#include <vector>
#include <string>
#include <cstdint>

// ── Common Data Structures ──────────────────────────────────────────────────

struct LangItem {
    std::string code;
    std::string name;
};

struct KBLayout {
    std::string code;
    std::string name;
};

struct ProfileOption {
    std::string name;
    std::string description;
    std::string default_de;
};

struct MirrorItem {
    std::string url;
    std::string country;
    bool        enabled;
};

struct ReflectorConfig {
    std::string countries = "Worldwide";
    int last_n = 20;
    std::string sort = "rate";
};

struct PacmanSettings {
    bool parallel_downloads = true;
    int max_parallel = 5;
    bool color = true;
    bool check_space = true;
    bool verbose_pkg_lists = true;
};

struct DiskPartition {
    std::string device;
    std::string mount_point;
    uint64_t    size_mb;
    std::string filesystem;
    std::string flags;
};

struct DiskInfo {
    std::string device;
    std::string model;
    uint64_t    size_mb;
    std::string table_type;
    std::vector<DiskPartition> partitions;
};

struct KernelOption {
    std::string package;
    std::string description;
    bool        selected;
};

// ── DataStore Singleton ─────────────────────────────────────────────────────

class DataStore {
public:
    static DataStore& instance() {
        static DataStore inst;
        return inst;
    }

    // Language & Locale
    std::vector<LangItem> languages;
    int selected_language_idx = 0;
    
    std::vector<KBLayout> keyboard_layouts;
    int selected_kb_idx = 0;
    
    std::vector<std::string> locales;
    std::vector<std::string> selected_locales;

    // Mirror Configuration
    std::vector<MirrorItem> mirrors;
    ReflectorConfig reflector_cfg;
    PacmanSettings pacman_cfg;

    // Storage Configuration
    std::vector<DiskInfo> disks;

    // Kernels
    std::vector<KernelOption> kernels;

    // Profiles
    std::vector<ProfileOption> profiles;
    int selected_profile_idx = 0;

    // Performance Settings
    bool zram_enabled = false;
    bool zswap_enabled = false;

    // ... Add more as needed ...

private:
    DataStore() = default;
    DataStore(const DataStore&) = delete;
    DataStore& operator=(const DataStore&) = delete;
};
