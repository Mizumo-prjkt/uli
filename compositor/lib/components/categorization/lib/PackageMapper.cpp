#include "PackageMapper.hpp"
#include <QDebug>

namespace uli {
namespace compositor {
namespace categorization {

PackageMapper::PackageMapper() {}
PackageMapper::~PackageMapper() {}

void PackageMapper::addDistroPackages(const QString &distro, const QList<fetchers::PackageInfo> &packages) {
    repoData.insert(distro, packages);
}

QList<CompositeComponent> PackageMapper::findEquivalents(const QStringList &baseNames) {
    QList<CompositeComponent> results;
    
    for (const QString &base : baseNames) {
        CompositeComponent comp;
        comp.id = base.toLower().replace(" ", "-");
        comp.name = base;
        comp.description = "Auto-mapped component for " + base;
        comp.category = "General";
        
        // Basic heuristic: Exact name match across distros
        for (auto it = repoData.begin(); it != repoData.end(); ++it) {
            QString distro = it.key();
            const QList<fetchers::PackageInfo> &pkgs = it.value();
            
            for (const auto &p : pkgs) {
                if (p.name.compare(base, Qt::CaseInsensitive) == 0) {
                    comp.distroPackages[distro].append(p.name);
                    if (comp.description.contains("Auto-mapped") && !p.description.isEmpty()) {
                        comp.description = p.description;
                    }
                    break;
                }
            }
        }
        
        // Only add if we found it in at least one distro
        if (!comp.distroPackages.isEmpty()) {
            results.append(comp);
        }
    }
    
    return results;
}

} // namespace categorization
} // namespace compositor
} // namespace uli
