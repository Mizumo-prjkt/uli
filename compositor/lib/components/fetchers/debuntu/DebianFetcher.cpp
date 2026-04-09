#include "DebianFetcher.hpp"
#include <QDebug>
#include <archive.h>
#include <archive_entry.h>

namespace uli {
namespace compositor {
namespace fetchers {

DebianFetcher::DebianFetcher(QObject *parent) : RepoFetcher(parent) {
    connect(manager, &QNetworkAccessManager::finished, this, &DebianFetcher::onDownloadFinished);
}

DebianFetcher::~DebianFetcher() {}

void DebianFetcher::fetch(const QUrl &url) {
    emit statusUpdated("Starting Debian Packages fetch from: " + url.toString());
    QNetworkRequest request(url);
    manager->get(request);
}

void DebianFetcher::onDownloadFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred("Debian Download Error: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    emit statusUpdated(QString("Download complete (%1 bytes). Decompressing package stream...").arg(data.size()));
    parsePackages(data);
    reply->deleteLater();
}

void DebianFetcher::parsePackages(const QByteArray &data) {
    struct archive *a = archive_read_new();
    archive_read_support_filter_all(a);
    archive_read_support_format_raw(a); // Debian Packages.gz is often just raw compressed stream
    
    if (archive_read_open_memory(a, data.data(), data.size()) != ARCHIVE_OK) {
        emit errorOccurred("Failed to open Debian Packages archive.");
        archive_read_free(a);
        return;
    }

    struct archive_entry *entry;
    QByteArray uncompressedData;
    
    // Read the uncompressed stream
    if (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        char buffer[8192];
        while (true) {
            ssize_t bytesRead = archive_read_data(a, buffer, sizeof(buffer));
            if (bytesRead <= 0) break;
            uncompressedData.append(buffer, bytesRead);
        }
    }
    archive_read_free(a);

    if (uncompressedData.isEmpty()) {
        emit errorOccurred("Failed to decompress Debian Packages stream.");
        return;
    }

    QList<PackageInfo> packages;
    QString fullText = QString::fromUtf8(uncompressedData);
    QStringList stanzas = fullText.split("\n\n");

    for (const QString &stanza : stanzas) {
        if (stanza.trimmed().isEmpty()) continue;
        
        PackageInfo pkg;
        QStringList lines = stanza.split('\n');
        for (const QString &line : lines) {
            if (line.startsWith("Package:")) pkg.name = line.mid(8).trimmed();
            else if (line.startsWith("Version:")) pkg.version = line.mid(8).trimmed();
            else if (line.startsWith("Description:")) pkg.description = line.mid(12).trimmed();
            else if (line.startsWith("Depends:")) {
                QString depsLine = line.mid(8).trimmed();
                // Debian depends are comma-separated and often have version constraints
                pkg.dependencies = depsLine.split(',', Qt::SkipEmptyParts);
            }
        }
        
        if (!pkg.name.isEmpty()) {
            packages.append(pkg);
        }
    }

    emit fetchCompleted(packages);
    emit statusUpdated("Debian parsing complete. Found " + QString::number(packages.size()) + " packages.");
}

} // namespace fetchers
} // namespace compositor
} // namespace uli
