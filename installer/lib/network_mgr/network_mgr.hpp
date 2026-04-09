#ifndef ULI_NETWORK_MGR_HPP
#define ULI_NETWORK_MGR_HPP

#include <iostream>
#include <cstdlib>

namespace uli {
namespace network_mgr {

class NetworkManager {
public:
    // Pings a universally accessible IP (e.g., Google DNS 8.8.8.8) to verify basic ICMP routing out of the host.
    // '-c 1' sends 1 packet, '-W 2' sets a 2-second timeout
    static bool test_internet_connection() {
        std::string cmd = "ping -c 1 -W 2 8.8.8.8 > /dev/null 2>&1";
        return (std::system(cmd.c_str()) == 0);
    }
    
    // Checks standard DNS resolution (does bing.com resolve?) to ensure packages relying on nameservers can fetch details.
    static bool test_dns_resolution() {
        std::string cmd = "ping -c 1 -W 2 bing.com > /dev/null 2>&1";
        return (std::system(cmd.c_str()) == 0);
    }
};

} // namespace network_mgr
} // namespace uli

#endif // ULI_NETWORK_MGR_HPP
