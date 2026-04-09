#ifndef ULI_PARTITIONER_IDENTIFIERS_BLKID_WRAPPER_HPP
#define ULI_PARTITIONER_IDENTIFIERS_BLKID_WRAPPER_HPP

#include <string>
#include "../diskcheck.hpp"

namespace uli {
namespace partitioner {
namespace identifiers {

class BlkidWrapper {
public:
    // Fetches the PARTUUID for a newly created partition device boundary mapping
    static std::string get_partuuid(const std::string& device_path) {
        std::string cmd = "blkid -s PARTUUID -o value \"" + device_path + "\"";
        std::string output = uli::partitioner::DiskCheck::exec_command(cmd.c_str());
        // Clean trailing newlines
        if (!output.empty() && output.back() == '\n') {
            output.pop_back();
        }
        return output;
    }
    
    // Fallback UUID
    static std::string get_uuid(const std::string& device_path) {
        std::string cmd = "blkid -s UUID -o value \"" + device_path + "\"";
        std::string output = uli::partitioner::DiskCheck::exec_command(cmd.c_str());
        if (!output.empty() && output.back() == '\n') {
            output.pop_back();
        }
        return output;
    }
};

} // namespace identifiers
} // namespace partitioner
} // namespace uli

#endif // ULI_PARTITIONER_IDENTIFIERS_BLKID_WRAPPER_HPP
