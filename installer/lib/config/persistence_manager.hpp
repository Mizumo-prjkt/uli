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
    static constexpr const char* CHECKPOINT_PATH = "/tmp/uli_checkpoint.yaml";

    static bool save_checkpoint(const MenuState& state) {
        return ConfigExporter::save_menu_state_to_yaml(state, CHECKPOINT_PATH);
    }

    static bool load_checkpoint(MenuState& state) {
        if (!std::filesystem::exists(CHECKPOINT_PATH)) return false;
        return ConfigLoader::load_yaml_to_menu_state(CHECKPOINT_PATH, state);
    }

    static bool has_checkpoint() {
        return std::filesystem::exists(CHECKPOINT_PATH);
    }

    static void clear_checkpoint() {
        if (std::filesystem::exists(CHECKPOINT_PATH)) {
            std::filesystem::remove(CHECKPOINT_PATH);
        }
    }
};


} // namespace config
} // namespace runtime
} // namespace uli

#endif // ULI_CONFIG_PERSISTENCE_MANAGER_HPP
