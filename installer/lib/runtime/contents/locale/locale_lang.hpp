#ifndef ULI_RUNTIME_LOCALE_LANG_HPP
#define ULI_RUNTIME_LOCALE_LANG_HPP

#include <string>
#include <vector>

namespace uli {
namespace runtime {
namespace contents {
namespace locale {

class LocaleLang {
public:
    // Returns a comprehensive list of standard OS Locale identifiers
    static std::vector<std::string> get_languages() {
        return {
            "en_US", "en_GB", "en_AU", "en_CA", 
            "es_ES", "es_MX", "es_AR", "fr_FR", "fr_CA", 
            "de_DE", "de_AT", "de_CH", "it_IT", "pt_PT", "pt_BR", 
            "ru_RU", "ja_JP", "zh_CN", "zh_TW", "ko_KR", 
            "nl_NL", "pl_PL", "sv_SE", "tr_TR", "ar_AE"
        };
    }
};

} // namespace locale
} // namespace contents
} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_LOCALE_LANG_HPP
