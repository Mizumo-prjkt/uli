#include "ArchFetcher.hpp"
#include <QDebug>
#include <archive.h>
#include <archive_entry.h>

namespace uli {
namespace compositor {
namespace fetchers {

ArchFetcher::ArchFetcher(QObject *parent) : RepoFetcher(parent) {
    connect(manager, &QNetworkAccessManager::finished, this, &ArchFetcher::onDownloadFinished);
}

ArchFetcher::~ArchFetcher() {}

void ArchFetcher::fetch(const QUrl &url) {
    emit statusUpdated("Starting Arch repository fetch from: " + url.toString());
    QNetworkRequest request(url);
    manager->get(request);
}

void ArchFetcher::onDownloadFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("Arch Download Error: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    emit statusUpdated("Download complete. Parsing database...");
    parseDatabase(data);
    reply->deleteLater();
}

void ArchFetcher::parseDatabase(const QByteArray &data) {
    struct archive *a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    
    if (archive_read_open_memory(a, data.data(), data.size()) != ARCHIVE_OK) {
        emit errorOccurred("Failed to open Arch database in memory.");
        archive_read_free(a);
        return;
    }

    QList<PackageInfo> packages;
    struct archive_entry *entry;
    
    QMap<QString, PackageInfo> pkgMap;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        QString entryPath = QString::fromUtf8(archive_entry_pathname(entry));
        if (entryPath.endsWith("/desc")) {
            size_t size = archive_entry_size(entry);
            QByteArray content(size, 0);
            archive_read_data(a, content.data(), size);
            
            QString contentStr = QString::fromUtf8(content);
            QStringList lines = contentStr.split('\n');
            
            PackageInfo pkg;
            QString currentTag;
            
            for (const QString &line : lines) {
                if (line.startsWith('%') && line.endsWith('%')) {
                    currentTag = line;
                } else if (!line.trimmed().isEmpty()) {
                    if (currentTag == "%NAME%") pkg.name = line.trimmed();
                    else if (currentTag == "%VERSION%") pkg.version = line.trimmed();
                    else if (currentTag == "%DESC%") pkg.description = line.trimmed();
                    else if (currentTag == "%DEPENDS%") pkg.dependencies << line.trimmed();
                }
            }
            
            if (!pkg.name.isEmpty()) {
                pkgMap.insert(pkg.name, pkg);
            }
        }
    }

    archive_read_free(a);
    emit fetchCompleted(pkgMap.values());
    emit statusUpdated("Arch parsing complete. Found " + QString::number(pkgMap.size()) + " packages.");
}

} // namespace fetchers
} // namespace compositor
} // namespace uli
