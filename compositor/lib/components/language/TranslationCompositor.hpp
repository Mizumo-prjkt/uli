#ifndef ULI_COMPOSITOR_TRANSLATION_COMPOSITOR_HPP
#define ULI_COMPOSITOR_TRANSLATION_COMPOSITOR_HPP

#include "categorization/lib/PackageMapper.hpp"
#include <QMap>
#include <QString>

namespace uli {
namespace compositor {
namespace language {

class TranslationCompositor {
public:
    TranslationCompositor();
    ~TranslationCompositor();

    void injectAutostart(categorization::CompositeComponent &comp);
    void mergeTranslations(categorization::CompositeComponent &comp, const QMap<QString, QString> &translations);

private:
    QStringList getSystemdCommands(const QString &pkg);
    QStringList getOpenRCCommands(const QString &pkg);
    QStringList getSysVinitCommands(const QString &pkg);
};

} // namespace language
} // namespace compositor
} // namespace uli

#endif // ULI_COMPOSITOR_TRANSLATION_COMPOSITOR_HPP
