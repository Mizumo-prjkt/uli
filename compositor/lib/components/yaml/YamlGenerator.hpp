#ifndef ULI_COMPOSITOR_YAML_GENERATOR_HPP
#define ULI_COMPOSITOR_YAML_GENERATOR_HPP

#include "categorization/lib/PackageMapper.hpp"
#include <QString>
#include <QList>

namespace uli {
namespace compositor {
namespace yaml {

class YamlGenerator {
public:
    static bool saveToYaml(const QString &path, const QList<categorization::CompositeComponent> &components);
};

} // namespace yaml
} // namespace compositor
} // namespace uli

#endif // ULI_COMPOSITOR_YAML_GENERATOR_HPP
