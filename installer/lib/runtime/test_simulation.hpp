#ifndef ULI_RUNTIME_TEST_SIMULATION_HPP
#define ULI_RUNTIME_TEST_SIMULATION_HPP

#include <string>

namespace uli {
namespace runtime {

struct TestSimulationConfig {
    bool enabled = false;
    std::string distribution = "alpine"; // e.g. debian, arch, alpine
    int distro_version = 0; // e.g. 12, 13 for debian:12, debian:13. 0 = auto-detect
    std::string disk_size = "500G";
    std::string disk_type = "nvme"; // e.g. nvme, sata
    std::string hardware = "amd"; // e.g. amd, intel
    bool no_masking = false;
    bool fetch_repos = false;
};

class TestSimulation {
public:
    static TestSimulationConfig& get_config() {
        static TestSimulationConfig config;
        return config;
    }

    static void enable() { get_config().enabled = true; }
    static bool is_enabled() { return get_config().enabled; }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_TEST_SIMULATION_HPP
