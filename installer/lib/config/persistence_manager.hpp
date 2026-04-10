#ifndef ULI_CONFIG_PERSISTENCE_MANAGER_HPP
#define ULI_CONFIG_PERSISTENCE_MANAGER_HPP

#include "config_exporter.hpp"
#include "config_loader.hpp"
#include <string>
#include <filesystem>

namespace uli {
namespace runtime {
namespace config {

class PersistenceManager {
public:
    static constexpr const char* RECOVERY_PATH = "/tmp/uli_recovery.yaml";

    static bool save_recovery(const MenuState& state) {
        return ConfigExporter::save_menu_state_to_yaml(state, RECOVERY_PATH);
    }

    static bool load_recovery(MenuState& state) {
        if (!std::filesystem::exists(RECOVERY_PATH)) return false;
        return ConfigLoader::load_yaml_to_menu_state(RECOVERY_PATH, state);
    }

    static bool has_recovery() {
        return std::filesystem::exists(RECOVERY_PATH);
    }

    static void clear_recovery() {
        if (std::filesystem::exists(RECOVERY_PATH)) {
            std::filesystem::remove(RECOVERY_PATH);
        }
    }
};

} // namespace config
} // namespace runtime
} // namespace uli

#endif // ULI_CONFIG_PERSISTENCE_MANAGER_HPP
