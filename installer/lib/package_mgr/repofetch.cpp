#include "repofetch.hpp"
#include <map>

namespace uli {
namespace debug {

// --- Arch Linux Implementation ---
bool ArchDebugFetcher::fetch_and_parse(const std::string& url) {
#ifdef ULI_HAS_LIBARCHIVE
    std::string temp_file = download_to_temp(url);
    if (temp_file.empty()) return false;

    struct archive *a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    
    if (archive_read_open_filename(a, temp_file.c_str(), 16384) != ARCHIVE_OK) {
        archive_read_free(a);
        std::filesystem::remove(temp_file);
        return false;
    }

    struct archive_entry *entry;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        std::string entryPath = archive_entry_pathname(entry);
        if (entryPath.size() > 5 && entryPath.substr(entryPath.size() - 5) == "/desc") {
            size_t size = archive_entry_size(entry);
            std::string content(size, 0);
            archive_read_data(a, &content[0], size);
            
            std::istringstream stream(content);
            std::string line;
            PackageInfo pkg;
            std::string currentTag;
            
            while (std::getline(stream, line)) {
                if (!line.empty() && line[0] == '%' && line[line.size()-1] == '%') {
                    currentTag = line;
                } else if (!line.empty()) {
                    if (currentTag == "%NAME%") pkg.name = line;
                    else if (currentTag == "%VERSION%") pkg.version = line;
                    else if (currentTag == "%DESC%") pkg.description = line;
                    else if (currentTag == "%DEPENDS%") pkg.dependencies.push_back(line);
                }
            }
            if (!pkg.name.empty()) packages.push_back(pkg);
        }
    }

    archive_read_free(a);
    std::filesystem::remove(temp_file);
    return true;
#else
    std::cerr << "[ERROR] libarchive support NOT compiled in. Arch fetching disabled." << std::endl;
    return false;
#endif
}

// --- Debian/Ubuntu Implementation ---
bool DebianDebugFetcher::fetch_and_parse(const std::string& url) {
#ifdef ULI_HAS_LIBARCHIVE
    std::string temp_file = download_to_temp(url);
    if (temp_file.empty()) return false;

    std::string uncompressed = decompress_raw_stream(temp_file);
    std::filesystem::remove(temp_file);

    if (uncompressed.empty()) return false;

    std::istringstream full_stream(uncompressed);
    std::string line;
    PackageInfo pkg;
    
    while (std::getline(full_stream, line)) {
        if (line.empty() || line == "\r") {
            if (!pkg.name.empty()) {
                packages.push_back(pkg);
                pkg = PackageInfo();
            }
            continue;
        }

        if (line.find("Package: ") == 0) pkg.name = line.substr(9);
        else if (line.find("Version: ") == 0) pkg.version = line.substr(9);
        else if (line.find("Description: ") == 0) pkg.description = line.substr(13);
        else if (line.find("Depends: ") == 0) {
            std::string deps = line.substr(9);
            // Simple split by comma
            std::istringstream dstream(deps);
            std::string d;
            while (std::getline(dstream, d, ',')) {
                if (!d.empty() && d[0] == ' ') d.erase(0, 1);
                pkg.dependencies.push_back(d);
            }
        }
    }
    if (!pkg.name.empty()) packages.push_back(pkg);
    return true;
#else
    return false;
#endif
}

// --- Alpine Linux Implementation ---
bool AlpineDebugFetcher::fetch_and_parse(const std::string& url) {
#ifdef ULI_HAS_LIBARCHIVE
    std::string temp_file = download_to_temp(url);
    if (temp_file.empty()) return false;

    struct archive *a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    
    if (archive_read_open_filename(a, temp_file.c_str(), 16384) != ARCHIVE_OK) {
        archive_read_free(a);
        std::filesystem::remove(temp_file);
        return false;
    }

    struct archive_entry *entry;
    std::string index_content;
    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        std::string entryPath = archive_entry_pathname(entry);
        if (entryPath == "APKINDEX") {
            size_t size = archive_entry_size(entry);
            index_content.resize(size);
            archive_read_data(a, &index_content[0], size);
            break;
        }
    }
    archive_read_free(a);
    std::filesystem::remove(temp_file);

    if (index_content.empty()) return false;

    std::istringstream full_stream(index_content);
    std::string line;
    PackageInfo pkg;

    while (std::getline(full_stream, line)) {
        if (line.empty() || line == "\r") {
            if (!pkg.name.empty()) {
                packages.push_back(pkg);
                pkg = PackageInfo();
            }
            continue;
        }

        if (line.find("P:") == 0) pkg.name = line.substr(2);
        else if (line.find("V:") == 0) pkg.version = line.substr(2);
        else if (line.find("T:") == 0) pkg.description = line.substr(2);
        else if (line.find("D:") == 0) {
            std::string deps = line.substr(2);
            std::istringstream dstream(deps);
            std::string d;
            while (std::getline(dstream, d, ' ')) {
                pkg.dependencies.push_back(d);
            }
        }
    }
    if (!pkg.name.empty()) packages.push_back(pkg);
    return true;
#else
    return false;
#endif
}

} // namespace debug
} // namespace uli
