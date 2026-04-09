#include "AlpineFetcher.hpp"
#include <QDebug>
#include <archive.h>
#include <archive_entry.h>

namespace uli {
namespace compositor {
namespace fetchers {

AlpineFetcher::AlpineFetcher(QObject *parent) : RepoFetcher(parent) {
    connect(manager, &QNetworkAccessManager::finished, this, &AlpineFetcher::onDownloadFinished);
}

AlpineFetcher::~AlpineFetcher() {}

void AlpineFetcher::fetch(const QUrl &url) {
    emit statusUpdated("Starting Alpine APK index fetch from: " + url.toString());
    QNetworkRequest request(url);
    manager->get(request);
}

void AlpineFetcher::onDownloadFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("Alpine Download Error: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    emit statusUpdated("Download complete. Decompressing APKINDEX...");
    parseIndex(data);
    reply->deleteLater();
}

void AlpineFetcher::parseIndex(const QByteArray &data) {
    struct archive *a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);
    
    if (archive_read_open_memory(a, data.data(), data.size()) != ARCHIVE_OK) {
        emit errorOccurred("Failed to open Alpine archive in memory.");
        archive_read_free(a);
        return;
    }

    struct archive_entry *entry;
    QByteArray indexContent;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        QString entryPath = QString::fromUtf8(archive_entry_pathname(entry));
        if (entryPath == "APKINDEX") {
            size_t size = archive_entry_size(entry);
            indexContent.resize(size);
            archive_read_data(a, indexContent.data(), size);
            break;
        }
    }
    archive_read_free(a);

    if (indexContent.isEmpty()) {
        emit errorOccurred("APKINDEX file not found in the archive.");
        return;
    }

    QList<PackageInfo> packages;
    QString fullText = QString::fromUtf8(indexContent);
    QStringList stanzas = fullText.split("\n\n");

    for (const QString &stanza : stanzas) {
        if (stanza.trimmed().isEmpty()) continue;
        
        PackageInfo pkg;
        QStringList lines = stanza.split('\n');
        for (const QString &line : lines) {
            if (line.startsWith("P:")) pkg.name = line.mid(2).trimmed();
            else if (line.startsWith("V:")) pkg.version = line.mid(2).trimmed();
            else if (line.startsWith("T:")) pkg.description = line.mid(2).trimmed();
            else if (line.startsWith("D:")) {
                QString depsLine = line.mid(2).trimmed();
                pkg.dependencies = depsLine.split(' ', Qt::SkipEmptyParts);
            }
        }
        
        if (!pkg.name.isEmpty()) {
            packages.append(pkg);
        }
    }

    emit fetchCompleted(packages);
    emit statusUpdated("Alpine parsing complete. Found " + QString::number(packages.size()) + " packages.");
}

} // namespace fetchers
} // namespace compositor
} // namespace uli
