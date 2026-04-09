#include "YamlGenerator.hpp"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <QDateTime>

namespace uli {
namespace compositor {
namespace yaml {

bool YamlGenerator::saveToYaml(const QString &path, const QList<categorization::CompositeComponent> &components) {
    YAML::Node root;
    root["version"] = "1.0";
    root["metadata"]["generated_at"] = QDateTime::currentDateTime().toString(Qt::ISODate).toStdString();
    
    YAML::Node compsNode;
    for (const auto &comp : components) {
        YAML::Node c;
        c["id"] = comp.id.toStdString();
        c["name"] = comp.name.toStdString();
        c["description"] = comp.description.toStdString();
        c["category"] = comp.category.toStdString();
        
        // Packages
        for (auto it = comp.distroPackages.begin(); it != comp.distroPackages.end(); ++it) {
            for (const QString &pkg : it.value()) {
                c["packages"][it.key().toStdString()].push_back(pkg.toStdString());
            }
        }
        
        // Autostart
        for (auto it = comp.autostart.begin(); it != comp.autostart.end(); ++it) {
            for (const QString &cmd : it.value()) {
                c["autostart"][it.key().toStdString()].push_back(cmd.toStdString());
            }
        }
        
        compsNode.push_back(c);
    }
    
    root["components"] = compsNode;
    
    std::ofstream fout(path.toStdString());
    if (!fout.is_open()) return false;
    
    fout << root;
    fout.close();
    
    return true;
}

} // namespace yaml
} // namespace compositor
} // namespace uli
