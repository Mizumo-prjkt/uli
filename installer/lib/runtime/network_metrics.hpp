#ifndef ULI_RUNTIME_NETWORK_METRICS_HPP
#define ULI_RUNTIME_NETWORK_METRICS_HPP

#include <iostream>
#include "../network_mgr/network_mgr.hpp"
#include "warn.hpp"
#include "dialogbox.hpp"

namespace uli {
namespace runtime {

class NetworkMetrics {
public:
    // Visually tests the system network state, warning the user if required
    static bool run_diagnostics() {
        DesignUI::print_header("Network Discovery");
        
        std::cout << "\nChecking external ICMP connectivity (8.8.8.8)... ";
        if (uli::network_mgr::NetworkManager::test_internet_connection()) {
            std::cout << DesignUI::GREEN << "[OK]" << DesignUI::RESET << "\n";
        } else {
            std::cout << DesignUI::RED << "[FAILED]" << DesignUI::RESET << "\n";
            Warn::print_error("Cannot route outward traffic. Please check your network adapter.");
            return false;
        }

        std::cout << "Checking DNS Nameserver Resolution... ";
        if (uli::network_mgr::NetworkManager::test_dns_resolution()) {
            std::cout << DesignUI::GREEN << "[OK]" << DesignUI::RESET << "\n\n";
        } else {
            std::cout << DesignUI::RED << "[FAILED]" << DesignUI::RESET << "\n\n";
            Warn::print_error("DNS fails to resolve! Mirrors will be unreachable.");
            return false;
        }
        
        Warn::print_info("Network link verified successfully.");
        return true;
    }
};

} // namespace runtime
} // namespace uli

#endif // ULI_RUNTIME_NETWORK_METRICS_HPP
