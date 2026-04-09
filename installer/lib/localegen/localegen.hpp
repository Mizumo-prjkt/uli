#ifndef ULI_LOCALENGEN_HPP
#define ULI_LOCALENGEN_HPP

#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <vector>
#include <iostream>
#include "../chroot_hook/chroot_hook.hpp"

namespace uli {
namespace localegen {

class LocaleGenerator {
public:
    // Configures the locale definition files and invokes system generation compilers
    static bool generate_locales(const std::string& chroot_mount, const std::string& lang, const std::string& encoding) {
        if (lang.empty() || encoding.empty()) return false;
        
        std::string locale_str = lang + "." + encoding;
        std::string locale_conf = chroot_mount + "/etc/locale.conf";
        std::string locale_gen = chroot_mount + "/etc/locale.gen";
        
        // 1. Write locale.conf mapping the default active fallback LANG
        std::ofstream conf_out(locale_conf);
        if (!conf_out.is_open()) return false;
        conf_out << "LANG=" << locale_str << "\n";
        conf_out.close();

        // 2. Identify and uncomment the target locale in /etc/locale.gen if it exists
        // (This behavior caters directly to Arch Linux's format)
        std::ifstream gen_in(locale_gen);
        if (gen_in.is_open()) {
            std::string line;
            std::vector<std::string> lines;
            bool found = false;

            while (std::getline(gen_in, line)) {
                // If the commented line matches our target locale identifier exactly
                if (line.find("#" + locale_str) == 0 || line.find("# " + locale_str) == 0) {
                    lines.push_back(locale_str + " " + encoding);
                    found = true;
                } else {
                    lines.push_back(line);
                }
            }
            gen_in.close();

            // If we didn't find a matching hashed pattern, we manually append it for OS's that dont ship full lists
            if (!found) {
                lines.push_back(locale_str + " " + encoding);
            }

            // Flush the parsed changes backward over the old locale.gen
            std::ofstream gen_out(locale_gen, std::ios::trunc);
            if (!gen_out.is_open()) return false;
            for (const auto& l : lines) {
                gen_out << l << "\n";
            }
            gen_out.close();
        } else {
            // Target file did not exist (e.g., minimalist OS without a pre-baked list to strip). Attempt raw generation.
            std::ofstream gen_out(locale_gen);
            if (gen_out.is_open()) {
                gen_out << locale_str << " " << encoding << "\n";
                gen_out.close();
            } else {
                return false;
            }
        }

        // 3. Compile the locale maps natively using chroot.
        // If `locale-gen` is absent (like Alpine), the command silently fails gracefully leaving our .conf fallback.
        std::string compile_cmd = "locale-gen";
        uli::hook::UniversalChroot::ScopedChroot(chroot_mount).execute(compile_cmd);

        return true;
    }
};

} // namespace localegen
} // namespace uli

#endif // ULI_LOCALENGEN_HPP
