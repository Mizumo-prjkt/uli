#include "SyncManager.hpp"
#include <QDebug>

namespace uli {
namespace compositor {

SyncManager::SyncManager(QObject *parent) : QObject(parent), m_finishedCount(0) {
    mapper = new categorization::PackageMapper();
    compositor = new language::TranslationCompositor();
    
    // Core + Extra
    m_sources << RepoSource{"Arch Linux", "core", QUrl("https://mirror.osbeck.com/archlinux/core/os/x86_64/core.db"), RepoSource::ARCH};
    m_sources << RepoSource{"Arch Linux", "extra", QUrl("https://mirror.osbeck.com/archlinux/extra/os/x86_64/extra.db"), RepoSource::ARCH};
    
    // Main + Community
    m_sources << RepoSource{"Alpine Linux", "main", QUrl("https://dl-cdn.alpinelinux.org/alpine/v3.19/main/x86_64/APKINDEX.tar.gz"), RepoSource::ALPINE};
    m_sources << RepoSource{"Alpine Linux", "community", QUrl("https://dl-cdn.alpinelinux.org/alpine/v3.19/community/x86_64/APKINDEX.tar.gz"), RepoSource::ALPINE};
    
    // Debian Trixie: Main + Security + Updates
    m_sources << RepoSource{"Debian/Ubuntu", "main", QUrl("http://ftp.debian.org/debian/dists/trixie/main/binary-amd64/Packages.xz"), RepoSource::DEBIAN};
    m_sources << RepoSource{"Debian/Ubuntu", "security", QUrl("http://security.debian.org/debian-security/dists/trixie-security/main/binary-amd64/Packages.xz"), RepoSource::DEBIAN};
    m_sources << RepoSource{"Debian/Ubuntu", "updates", QUrl("http://ftp.debian.org/debian/dists/trixie-updates/main/binary-amd64/Packages.xz"), RepoSource::DEBIAN};

    m_totalCount = m_sources.size();
}

SyncManager::~SyncManager() {
    delete mapper;
    delete compositor;
}

void SyncManager::startSync() {
    m_finishedCount = 0;
    m_archPkgs.clear();
    m_alpinePkgs.clear();
    m_debianPkgs.clear();
    
    emit logMessage(QString("Initiating parallel synchronization for %1 repositories...").arg(m_totalCount));
    
    for (const auto &src : m_sources) {
        fetchers::RepoFetcher *fetcher = nullptr;
        if (src.type == RepoSource::ARCH) fetcher = new fetchers::ArchFetcher(this);
        else if (src.type == RepoSource::ALPINE) fetcher = new fetchers::AlpineFetcher(this);
        else if (src.type == RepoSource::DEBIAN) fetcher = new fetchers::DebianFetcher(this);

        if (fetcher) {
            connect(fetcher, &fetchers::RepoFetcher::fetchCompleted, this, [this, src, fetcher](const QList<fetchers::PackageInfo> &pkgs) {
                // Add repo badge to each package
                QList<fetchers::PackageInfo> badged = pkgs;
                for (auto &p : badged) p.repo = src.repoLabel;

                if (src.distro == "Arch Linux") m_archPkgs.append(badged);
                else if (src.distro == "Alpine Linux") m_alpinePkgs.append(badged);
                else if (src.distro == "Debian/Ubuntu") m_debianPkgs.append(badged);

                mapper->addDistroPackages(src.distro, badged);
                emit logMessage(QString("%1 [%2]: Found %3 packages.").arg(src.distro).arg(src.repoLabel).arg(pkgs.size()));
                
                fetcher->deleteLater();
                checkCompletion();
            });

            connect(fetcher, &fetchers::RepoFetcher::errorOccurred, this, [this, src, fetcher](const QString &err) {
                emit logMessage(QString("[ERROR] %1 [%2]: %3").arg(src.distro).arg(src.repoLabel).arg(err));
                fetcher->deleteLater();
                checkCompletion(); // Continue despite error
            });

            fetcher->fetch(src.url);
        }
    }
}

void SyncManager::onFetchCompleted(const QList<fetchers::PackageInfo> &) {
    // Handled in lambda above
}

void SyncManager::checkCompletion() {
    m_finishedCount++;
    emit progressUpdated((m_finishedCount * 100) / m_totalCount);
    
    if (m_finishedCount >= m_totalCount) {
        emit logMessage("Cross-distro mapping stabilization in progress...");
        
        QStringList baseNames = {"linux", "gcc", "bash", "vim", "python3", "nginx", "systemd", "openrc", "plasma-desktop", "gnome-shell"};
        m_finalComponents = mapper->findEquivalents(baseNames);
        
        emit logMessage("Synchronization complete.");
        emit syncFinished(m_finalComponents);
    }
}

} // namespace compositor
} // namespace uli
