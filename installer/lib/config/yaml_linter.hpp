#ifndef ULI_YAML_LINTER_HPP
#define ULI_YAML_LINTER_HPP

#include <string>
#include <vector>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <filesystem>

namespace uli {
namespace runtime {
namespace config {

enum class LintSeverity { INFO, WARNING, CRITICAL, FATAL };

struct LintIssue {
    LintSeverity severity;
    std::string message;
};

class YamlLinter {
public:
    static std::vector<LintIssue> lint(const std::string& profile_path, const std::string& dict_path, bool dict_provided_explicitly, bool using_builtin = false) {
        std::vector<LintIssue> issues;
        YAML::Node profile;

        // 0. Dictionary Status Info
        if (!dict_path.empty()) {
            if (using_builtin) {
                issues.push_back({LintSeverity::INFO, "No external dictionary found. Relying on Onboard Instructions (embedded " + dict_path + ")"});
            } else if (!dict_provided_explicitly) {
                issues.push_back({LintSeverity::INFO, "No --dict provided; using default found at: " + dict_path});
            }
        }

        // 1. Syntax Check
        try {
            if (!std::filesystem::exists(profile_path)) {
                issues.push_back({LintSeverity::FATAL, "Profile file not found: " + profile_path});
                return issues;
            }
            profile = YAML::LoadFile(profile_path);
        } catch (const YAML::Exception& e) {
            issues.push_back({LintSeverity::FATAL, "YAML Syntax Error: " + std::string(e.what())});
            return issues;
        }

        // 2. Required Fields (Fatal)
        std::vector<std::string> required = {"hostname", "root_password", "language"};
        for (const auto& key : required) {
            if (!profile[key] || profile[key].as<std::string>().empty()) {
                issues.push_back({LintSeverity::FATAL, "Missing required field: '" + key + "'"});
            }
        }

        // Gather metrics for cross-validation
        bool has_manual_linkages = (profile["custom_translations"] && profile["custom_translations"].IsSequence() && profile["custom_translations"].size() > 0);
        bool has_additional_pkgs = (profile["additional_packages"] && profile["additional_packages"].IsSequence() && profile["additional_packages"].size() > 0);
        bool has_dict = !dict_path.empty() && std::filesystem::exists(dict_path);
        bool has_partitions = (profile["disk_partitions"] && profile["disk_partitions"].IsSequence() && profile["disk_partitions"].size() > 0);

        // 3. User Requested Cross-Validation Rules

        // Rule 1: Manual linkages but no dict
        if (has_manual_linkages && !dict_provided_explicitly && !using_builtin) {
            issues.push_back({LintSeverity::WARNING, "Manual package linkages found without a translator (--dict). Behavior on install may be inconsistent if mappings are incomplete."});
        }

        // Rule 2: Manual linkages and dict present
        if (has_manual_linkages && dict_provided_explicitly && has_dict) {
            issues.push_back({LintSeverity::INFO, "Redundancy detected: Both manual linkages and external dictionary present. Translator takes priority; manual linkages act as fallbacks."});
        }

        // Rule 3: Zero packages, zero linkages, zero dict
        if (!has_additional_pkgs && !has_manual_linkages && !dict_provided_explicitly) {
            issues.push_back({LintSeverity::CRITICAL, "No packages, manual linkages, or translations defined. Installation is restricted to host defaults and is likely to fail on differing environments."});
        }

        // Rule 4: Dict provided but zero linkages
        if (dict_provided_explicitly && has_dict && !has_manual_linkages) {
            issues.push_back({LintSeverity::CRITICAL, "Translator provided but manual fallback linkers are empty. If translation fails for a segment, there is no backup layer to prevent failure."});
        }

        // 4. Additional Semantic Checks
        if (!has_partitions) {
            issues.push_back({LintSeverity::WARNING, "No disk_partitions defined. Installer will attempt 'Intelligent auto-compute', which requires sufficient unallocated space."});
        }

        if (profile["users"] && profile["users"].IsSequence() && profile["users"].size() == 0) {
            issues.push_back({LintSeverity::WARNING, "No users defined. The system will only be accessible via the root account."});
        }

        return issues;
    }

    static void print_report(const std::vector<LintIssue>& issues) {
        if (issues.empty()) {
            std::cout << "\033[1;32m[OK] YAML Lint Check Passed - No issues found.\033[0m" << std::endl;
            return;
        }

        int errors = 0, warnings = 0, infos = 0, criticals = 0;
        std::cout << "\n--- ULI YAML Lint Report ---\n" << std::endl;

        for (const auto& issue : issues) {
            std::string prefix;
            switch (issue.severity) {
                case LintSeverity::INFO:
                    prefix = "\033[1;36m[INFO]\033[0m ";
                    infos++;
                    break;
                case LintSeverity::WARNING:
                    prefix = "\033[1;33m[WARN]\033[0m ";
                    warnings++;
                    break;
                case LintSeverity::CRITICAL:
                    prefix = "\033[1;31m[CRIT]\033[0m ";
                    criticals++;
                    break;
                case LintSeverity::FATAL:
                    prefix = "\033[1;37;41m[FATAL]\033[0m ";
                    errors++;
                    break;
            }
            std::cout << prefix << issue.message << std::endl;
        }

        std::cout << "\nSummary: " 
                  << "\033[1;37;41m" << errors << " Errors\033[0m, "
                  << "\033[1;31m" << criticals << " Critical\033[0m, "
                  << "\033[1;33m" << warnings << " Warnings\033[0m, "
                  << "\033[1;36m" << infos << " Info\033[0m" << std::endl;
    }

    static bool has_fatal(const std::vector<LintIssue>& issues) {
        for (const auto& i : issues) if (i.severity == LintSeverity::FATAL) return true;
        return false;
    }
};

} // namespace config
} // namespace runtime
} // namespace uli

#endif // ULI_YAML_LINTER_HPP
