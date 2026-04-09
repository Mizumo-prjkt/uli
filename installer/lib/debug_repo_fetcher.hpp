#ifndef ULI_DEBUG_REPO_FETCHER_HPP
#define ULI_DEBUG_REPO_FETCHER_HPP

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifdef ULI_HAS_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

namespace uli {
namespace debug {

struct PackageInfo {
  std::string name;
  std::string version;
  std::string description;
  std::vector<std::string> dependencies;
};

class DebugRepoFetcher {
public:
  virtual ~DebugRepoFetcher() = default;
  virtual bool fetch_and_parse(const std::string &url) = 0;

  const std::vector<PackageInfo> &get_packages() const { return packages; }

protected:
  std::vector<PackageInfo> packages;

  // Lightweight networking using popen + curl
  std::string download_to_temp(const std::string &url) {
    std::string temp_file =
        "/tmp/uli_repo_debug_" + std::to_string(rand()) + ".tmp";
    std::string cmd = "curl -L -s -o " + temp_file + " " + url;

    std::cout << "[DEBUG] Fetching: " << url << std::endl;
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
      std::cerr << "[ERROR] curl failed to download " << url << std::endl;
      return "";
    }
    return temp_file;
  }

#ifdef ULI_HAS_LIBARCHIVE
  // Shared decompression logic using libarchive
  std::string decompress_raw_stream(const std::string &filepath) {
    struct archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_raw(a);

    if (archive_read_open_filename(a, filepath.c_str(), 16384) != ARCHIVE_OK) {
      archive_read_free(a);
      return "";
    }

    struct archive_entry *entry;
    std::string uncompressed;
    if (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
      char buffer[8192];
      while (true) {
        ssize_t bytes = archive_read_data(a, buffer, sizeof(buffer));
        if (bytes <= 0)
          break;
        uncompressed.append(buffer, bytes);
      }
    }
    archive_read_free(a);
    return uncompressed;
  }
#endif
};

// --- Distribution Implementations ---

class ArchDebugFetcher : public DebugRepoFetcher {
public:
  bool fetch_and_parse(const std::string &url) override;
};

class AlpineDebugFetcher : public DebugRepoFetcher {
public:
  bool fetch_and_parse(const std::string &url) override;
};

class DebianDebugFetcher : public DebugRepoFetcher {
public:
  bool fetch_and_parse(const std::string &url) override;
};

class FetcherFactory {
public:
  static std::unique_ptr<DebugRepoFetcher> create(const std::string &distro) {
    if (distro == "arch")
      return std::make_unique<ArchDebugFetcher>();
    if (distro == "alpine")
      return std::make_unique<AlpineDebugFetcher>();
    if (distro == "debian")
      return std::make_unique<DebianDebugFetcher>();
    return nullptr;
  }
};

} // namespace debug
} // namespace uli

#endif // ULI_DEBUG_REPO_FETCHER_HPP
