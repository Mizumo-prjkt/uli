#ifndef ULI_PARTITIONER_IDENTIFIERS_FLAG_PARTITIONS_HPP
#define ULI_PARTITIONER_IDENTIFIERS_FLAG_PARTITIONS_HPP

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include "../../distribution_checks.hpp"

namespace uli {
namespace partitioner {
namespace identifiers {

class FlagPartitions {
public:
    static std::string get_wiki_recommendation(const std::string& fs_type) {
        uli::checks::DistroType os_dist = uli::checks::detect_distribution();
        std::string rec = "For more information on " + fs_type + " mount flags, please consult ";
        
        switch (os_dist) {
            case uli::checks::DistroType::ARCH:
                rec += "the ArchWiki " + fs_type + " reference page.";
                break;
            case uli::checks::DistroType::DEBIAN:
                rec += "the Ubuntu/Debian community help wikis for " + fs_type + ", or the ArchWiki.";
                break;
            default:
                rec += "your distribution's documentation or the ArchWiki " + fs_type + " reference page.";
                break;
        }
        
        return rec;
    }

    static bool is_valid_option(const std::string& opt, const std::string& fs_type) {
        std::vector<std::string> common_flags = {"defaults", "noatime", "relatime", "ro", "rw", "sync", "async", "nodev", "nosuid", "noexec", "auto", "noauto", "user", "nouser", "exec", "suid", "dev", "diratime", "nodiratime", "strictatime", "lazytime"};
        
        std::vector<std::string> ext4_flags = {"acl", "noacl", "barrier=0", "barrier=1", "commit", "data=journal", "data=ordered", "data=writeback", "discard", "nodiscard", "errors=remount-ro", "errors=continue", "errors=panic", "grpid", "bsdgroups", "nogrpid", "sysvgroups", "journal_async_commit", "journal_checksum", "nouid32", "oldalloc", "orlov", "user_xattr", "nouser_xattr"};
        
        std::vector<std::string> btrfs_flags = {"alloc_start", "autodefrag", "noautodefrag", "check_int", "check_int_data", "check_int_print_mask", "commit", "compress", "compress=zlib", "compress=lzo", "compress=zstd", "compress-force", "compress-force=zlib", "compress-force=lzo", "compress-force=zstd", "degraded", "device", "discard", "discard=sync", "discard=async", "nodiscard", "enospc_debug", "fatal_errors=bug", "fatal_errors=panic", "flushoncommit", "noflushoncommit", "inode_cache", "max_inline", "metadata_ratio", "nologreplay", "norecovery", "rescan_uuid_tree", "skip_balance", "space_cache", "space_cache=v1", "space_cache=v2", "nospace_cache", "clear_cache", "ssd", "ssd_spread", "nossd", "noswapfile", "subvol", "subvolid", "thread_pool", "treelog", "notreelog", "usebackuproot", "user_subvol_rm_allowed"};
        
        std::vector<std::string> vfat_flags = {"check=r", "check=n", "check=s", "uid", "gid", "umask", "dmask", "fmask", "allow_utime", "codepage", "iocharset", "tz=UTC", "shortname=lower", "shortname=win95", "shortname=winnt", "shortname=mixed", "showexec", "debug", "fat", "quiet", "rodir", "sys_immutable", "flush", "usefree", "discard"};
        
        std::vector<std::string> xfs_flags = {"allocsize", "attr2", "noattr2", "barrier", "nobarrier", "discard", "nodiscard", "grpid", "bsdgroups", "nogrpid", "sysvgroups", "filestreams", "ikeep", "noikeep", "inode32", "inode64", "largeio", "nolargeio", "logbufs", "logbsize", "logdev", "noalign", "norecovery", "nouuid", "pquota", "prjquota", "pqnoenforce", "qnoenforce", "quota", "usrquota", "uqnoenforce", "swalloc", "sunit", "swidth"};

        std::vector<std::string> swap_flags = {"defaults", "pri", "discard"};

        auto check_list = [&opt](const std::vector<std::string>& list) {
            for (const auto& l : list) {
                if (opt == l) return true;
                // For parameterized options like compress=zstd or umask=0022
                if (l.find("=") != std::string::npos && opt.find(l.substr(0, l.find("=") + 1)) == 0) return true;
                // Catch dynamic ones like uid, gid
                if (l == "uid" && opt.find("uid=") == 0) return true;
                if (l == "gid" && opt.find("gid=") == 0) return true;
                if (l == "umask" && opt.find("umask=") == 0) return true;
                if (l == "dmask" && opt.find("dmask=") == 0) return true;
                if (l == "fmask" && opt.find("fmask=") == 0) return true;
                if (l == "subvol" && opt.find("subvol=") == 0) return true;
                if (l == "subvolid" && opt.find("subvolid=") == 0) return true;
            }
            return false;
        };

        if (check_list(common_flags)) return true;

        if (fs_type == "ext4" && check_list(ext4_flags)) return true;
        if (fs_type == "btrfs" && check_list(btrfs_flags)) return true;
        if (fs_type == "vfat" && check_list(vfat_flags)) return true;
        if (fs_type == "xfs" && check_list(xfs_flags)) return true;
        if (fs_type == "swap" && check_list(swap_flags)) return true;

        if (fs_type.empty() || fs_type == "unknown") { 
            return false; // Safely reject
        }

        return false; // Not found in allowed lists
    }

    struct LintResult {
        bool is_valid;
        std::string error_message;
    };

    static LintResult lint_options(const std::string& fs_type, const std::string& options_string) {
        if (options_string.empty()) {
            return {false, "Mount options cannot be empty. Use 'defaults' as a baseline."};
        }

        std::vector<std::string> options;
        std::stringstream ss(options_string);
        std::string item;
        while (std::getline(ss, item, ',')) {
            // Trim whitespace
            item.erase(item.begin(), std::find_if(item.begin(), item.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            item.erase(std::find_if(item.rbegin(), item.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), item.end());
            
            if (!item.empty()) {
                options.push_back(item);
            }
        }

        if (options.empty()) {
             return {false, "No valid options provided. Commas only are unsupported."};
        }

        for (const auto& opt : options) {
            if (!is_valid_option(opt, fs_type)) {
                return {false, "Unsupported or malformed option detected: '" + opt + "' for filesystem '" + fs_type + "'."};
            }
        }

        return {true, ""};
    }
};

} // namespace identifiers
} // namespace partitioner
} // namespace uli

#endif
