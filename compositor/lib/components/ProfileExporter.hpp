#ifndef ULI_COMPOSITOR_PROFILE_EXPORTER_HPP
#define ULI_COMPOSITOR_PROFILE_EXPORTER_HPP

#include <QString>
#include <QList>
#include <QStringList>
#include <yaml-cpp/yaml.h>
#include "storage/StorageModel.hpp"
#include "ui/ProfileEditor/UserDialog.hpp"

namespace uli {
namespace compositor {

struct LinkedPackage {
    QString genericName;
    QString archPkg;
    QString alpinePkg;
    QString debianPkg;
};

struct ProfileSpec {
    QString hostname;
    QString language;
    QString timezone;
    QString rootPassword;
    QString bootTarget;
    QString initSystem;
    QString targetDistro;
    bool isEfi;
    bool isAgnostic;
    int efiPart;
    QString efiDirectory;
    QString bootloaderId;
    QString networkBackend;
    QString wifiSsid;
    QString wifiPassphrase;
    
    QList<ui::UserSpec> users;
    QList<storage::PartitionConfig> partitions;
    QStringList additionalPkgs;
    QList<LinkedPackage> linkedPkgs;
};

class ProfileExporter {
public:
    static bool exportToYaml(const QString &path, const ProfileSpec &spec);
};

} // namespace compositor
} // namespace uli

#endif // ULI_COMPOSITOR_PROFILE_EXPORTER_HPP
