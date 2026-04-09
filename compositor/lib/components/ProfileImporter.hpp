#ifndef ULI_COMPOSITOR_PROFILE_IMPORTER_HPP
#define ULI_COMPOSITOR_PROFILE_IMPORTER_HPP

#include <QString>
#include "ProfileExporter.hpp"

namespace uli {
namespace compositor {

class ProfileImporter {
public:
    static bool importFromYaml(const QString &path, ProfileSpec &spec);
};

} // namespace compositor
} // namespace uli

#endif // ULI_COMPOSITOR_PROFILE_IMPORTER_HPP
