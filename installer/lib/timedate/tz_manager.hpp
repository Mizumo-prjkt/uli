#ifndef ULI_TIMEDATE_TZ_MANAGER_HPP
#define ULI_TIMEDATE_TZ_MANAGER_HPP

#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include "tz_dict.hpp"

namespace uli {
namespace timedate {

class TzManager {
public:
    static std::string to_lower(const std::string& str) {
        std::string res = str;
        std::transform(res.begin(), res.end(), res.begin(),
            [](unsigned char c){ return std::tolower(c); });
        return res;
    }

    static std::vector<std::string> get_all_timezones() {
        std::vector<std::string> paths;
        std::vector<std::string> regions = {"America", "Europe", "Asia", "Africa", "Australia", "Pacific", "Indian", "Atlantic", "Antarctica", "UTC", "Etc"};
        for (const auto& r : regions) {
            std::string root = "/usr/share/zoneinfo/" + r;
            if (std::filesystem::exists(root) && std::filesystem::is_directory(root)) {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
                    if (entry.is_regular_file()) {
                        std::string vp = entry.path().string().substr(20); // Length of /usr/share/zoneinfo/
                        paths.push_back(vp);
                    }
                }
            }
        }
        std::sort(paths.begin(), paths.end());
        return paths;
    }

    static std::vector<std::string> fuzzy_search(const std::string& query) {
        std::vector<std::string> results;
        if (query.empty()) return results;
        
        std::string lq = to_lower(query);
        
        // 1. Check dictionary matches (e.g. searching "Japan" finds "Asia/Tokyo")
        auto dict = TzDict::get_fuzzy_dictionary();
        for (const auto& pair : dict) {
            if (to_lower(pair.first).find(lq) != std::string::npos) {
                if (std::find(results.begin(), results.end(), pair.second) == results.end()) {
                    results.push_back(pair.second);
                }
            }
        }
        
        // 2. Check direct filesystem string matches (e.g. searching "Tokyo" finds "Asia/Tokyo")
        auto all = get_all_timezones();
        for (const auto& tz : all) {
            if (to_lower(tz).find(lq) != std::string::npos) {
                if (std::find(results.begin(), results.end(), tz) == results.end()) {
                    results.push_back(tz);
                }
            }
        }
        
        return results;
    }
};

} // namespace timedate
} // namespace uli

#endif // ULI_TIMEDATE_TZ_MANAGER_HPP
