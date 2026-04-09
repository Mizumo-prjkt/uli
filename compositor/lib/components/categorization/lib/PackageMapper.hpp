#ifndef ULI_COMPOSITOR_PACKAGE_MAPPER_HPP
#define ULI_COMPOSITOR_PACKAGE_MAPPER_HPP

#include <QString>
#include <QMap>
#include <QList>
#include "fetchers/RepoFetcher.hpp"

namespace uli {
namespace compositor {
namespace categorization {

struct CompositeComponent {
    QString id;
    QString name;
    QString description;
    QString category;
    
    QMap<QString, QStringList> distroPackages; // distro -> list of package names
    QMap<QString, QStringList> autostart;      // init-system -> list of commands
};

class PackageMapper {
public:
    PackageMapper();
    ~PackageMapper();

    void addDistroPackages(const QString &distro, const QList<fetchers::PackageInfo> &packages);
    QList<CompositeComponent> findEquivalents(const QStringList &baseNames);

private:
    QMap<QString, QList<fetchers::PackageInfo>> repoData;
};

} // namespace categorization
} // namespace compositor
} // namespace uli

#endif // ULI_COMPOSITOR_PACKAGE_MAPPER_HPP
