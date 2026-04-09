#ifndef ULI_COMPOSITOR_REPO_FETCHER_HPP
#define ULI_COMPOSITOR_REPO_FETCHER_HPP

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>
#include <QDir>

namespace uli {
namespace compositor {
namespace fetchers {

struct PackageInfo {
    QString name;
    QString version;
    QString description;
    QString repo;
    QStringList dependencies;
};

class RepoFetcher : public QObject {
    Q_OBJECT
public:
    explicit RepoFetcher(QObject *parent = nullptr);
    virtual ~RepoFetcher();

    virtual void fetch(const QUrl &url) = 0;

signals:
    void progressChanged(int percent);
    void statusUpdated(const QString &message);
    void fetchCompleted(const QList<PackageInfo> &packages);
    void errorOccurred(const QString &error);

protected:
    QNetworkAccessManager *manager;
};

} // namespace fetchers
} // namespace compositor
} // namespace uli

#endif // ULI_COMPOSITOR_REPO_FETCHER_HPP
