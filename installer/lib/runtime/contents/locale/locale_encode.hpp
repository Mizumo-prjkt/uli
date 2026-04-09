#ifndef ULI_RUNTIME_LOCALE_ENCODE_HPP
#define ULI_RUNTIME_LOCALE_ENCODE_HPP

#include <string>
#include <vector>

namespace uli {
namespace runtime {
namespace contents {
namespace locale {

class LocaleEncode {
public:
    // Returns standard OS String Encodings for Locale bindings
    static std::vector<std::string> get_encodings() {
        return {
            "UTF-8",
            "ISO-8859-1",
            "ISO-8859-15",
            "EUC-JP",
            "EUC-KR",
            "GB18030"
        };
    }
};

} // namespace locale
} // namespace contents
} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_LOCALE_ENCODE_HPP
