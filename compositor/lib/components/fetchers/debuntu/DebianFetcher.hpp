#ifndef ULI_COMPOSITOR_DEBIAN_FETCHER_HPP
#define ULI_COMPOSITOR_DEBIAN_FETCHER_HPP

#include "../RepoFetcher.hpp"

namespace uli {
namespace compositor {
namespace fetchers {

class DebianFetcher : public RepoFetcher {
    Q_OBJECT
public:
    explicit DebianFetcher(QObject *parent = nullptr);
    virtual ~DebianFetcher();

    void fetch(const QUrl &url) override;

private slots:
    void onDownloadFinished(QNetworkReply *reply);

private:
    void parsePackages(const QByteArray &data);
};

} // namespace fetchers
} // namespace compositor
} // namespace uli

#endif // ULI_COMPOSITOR_DEBIAN_FETCHER_HPP
