#ifndef ULI_COMPOSITOR_SYNC_MANAGER_HPP
#define ULI_COMPOSITOR_SYNC_MANAGER_HPP

#include <QObject>
#include <QList>
#include <QMap>
#include "fetchers/RepoFetcher.hpp"
#include "fetchers/arch/ArchFetcher.hpp"
#include "fetchers/alpine/AlpineFetcher.hpp"
#include "fetchers/debuntu/DebianFetcher.hpp"
#include "categorization/lib/PackageMapper.hpp"
#include "language/TranslationCompositor.hpp"

namespace uli {
namespace compositor {

struct RepoSource {
    QString distro;
    QString repoLabel;
    QUrl url;
    enum Type { ARCH, ALPINE, DEBIAN } type;
};

class SyncManager : public QObject {
    Q_OBJECT
public:
    explicit SyncManager(QObject *parent = nullptr);
    ~SyncManager();

    void startSync();

    QList<fetchers::PackageInfo> archPackages() const { return m_archPkgs; }
    QList<fetchers::PackageInfo> alpinePackages() const { return m_alpinePkgs; }
    QList<fetchers::PackageInfo> debianPackages() const { return m_debianPkgs; }

signals:
    void logMessage(const QString &msg);
    void progressUpdated(int percent);
    void syncFinished(const QList<categorization::CompositeComponent> &components);

private slots:
    void onFetchCompleted(const QList<fetchers::PackageInfo> &packages);
    void checkCompletion();

private:
    categorization::PackageMapper *mapper;
    language::TranslationCompositor *compositor;
    
    QList<RepoSource> m_sources;
    int m_finishedCount;
    int m_totalCount;

    QList<categorization::CompositeComponent> m_finalComponents;
    QList<fetchers::PackageInfo> m_archPkgs;
    QList<fetchers::PackageInfo> m_alpinePkgs;
    QList<fetchers::PackageInfo> m_debianPkgs;
};

} // namespace compositor
} // namespace uli

#endif // ULI_COMPOSITOR_SYNC_MANAGER_HPP
