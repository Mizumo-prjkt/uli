#ifndef ULI_ARGLINTER_HPP
#define ULI_ARGLINTER_HPP

#include <string>
#include <vector>
#include <algorithm>

namespace uli {
namespace arglinter {

inline int levenshtein_distance(const std::string& s1, const std::string& s2) {
    const std::size_t len1 = s1.size(), len2 = s2.size();
    std::vector<std::vector<int>> d(len1 + 1, std::vector<int>(len2 + 1));

    d[0][0] = 0;
    for(std::size_t i = 1; i <= len1; ++i) d[i][0] = static_cast<int>(i);
    for(std::size_t j = 1; j <= len2; ++j) d[0][j] = static_cast<int>(j);

    for(std::size_t i = 1; i <= len1; ++i) {
        for(std::size_t j = 1; j <= len2; ++j) {
            d[i][j] = std::min({
                d[i - 1][j] + 1,
                d[i][j - 1] + 1,
                d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1)
            });
        }
    }
    return d[len1][len2];
}

inline std::string suggest_flag(const std::string& invalid_flag, const std::vector<std::string>& valid_flags) {
    std::string best_match = "";
    int best_dist = -1;

    for (const auto& flag : valid_flags) {
        int dist = levenshtein_distance(invalid_flag, flag);
        if (best_dist == -1 || dist < best_dist) {
            best_dist = dist;
            best_match = flag;
        }
    }

    // Return the suggestion if it's reasonably close (e.g., distance <= 3)
    // and let's not suggest if it's the exact same string or distance is 0
    if (best_dist > 0 && best_dist <= 3) {
        return best_match;
    }
    return "";
}

} // namespace arglinter
} // namespace uli

#endif // ULI_ARGLINTER_HPP
