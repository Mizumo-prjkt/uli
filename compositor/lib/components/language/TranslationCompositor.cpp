#include "TranslationCompositor.hpp"

namespace uli {
namespace compositor {
namespace language {

TranslationCompositor::TranslationCompositor() {}
TranslationCompositor::~TranslationCompositor() {}

void TranslationCompositor::injectAutostart(categorization::CompositeComponent &comp) {
    // We assume the first package in the list is the primary service name
    QString primaryPkg;
    if (!comp.distroPackages["arch"].isEmpty()) primaryPkg = comp.distroPackages["arch"].first();
    else if (!comp.distroPackages["debian"].isEmpty()) primaryPkg = comp.distroPackages["debian"].first();
    else if (!comp.distroPackages["alpine"].isEmpty()) primaryPkg = comp.distroPackages["alpine"].first();

    if (primaryPkg.isEmpty()) return;

    comp.autostart["systemd"] = getSystemdCommands(primaryPkg);
    comp.autostart["openrc"] = getOpenRCCommands(primaryPkg);
    comp.autostart["sysvinit"] = getSysVinitCommands(primaryPkg);
}

void TranslationCompositor::mergeTranslations(categorization::CompositeComponent &comp, const QMap<QString, QString> &translations) {
    // TODO: Implement translation merging if needed in the component structure
}

QStringList TranslationCompositor::getSystemdCommands(const QString &pkg) {
    return { "systemctl enable --now " + pkg };
}

QStringList TranslationCompositor::getOpenRCCommands(const QString &pkg) {
    return { "rc-update add " + pkg + " default", "rc-service " + pkg + " start" };
}

QStringList TranslationCompositor::getSysVinitCommands(const QString &pkg) {
    return { "update-rc.d " + pkg + " defaults", "service " + pkg + " start" };
}

} // namespace language
} // namespace compositor
} // namespace uli
