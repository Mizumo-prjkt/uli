#ifndef ULI_COMPOSITOR_ALPINE_FETCHER_HPP
#define ULI_COMPOSITOR_ALPINE_FETCHER_HPP

#include "../RepoFetcher.hpp"

namespace uli {
namespace compositor {
namespace fetchers {

class AlpineFetcher : public RepoFetcher {
    Q_OBJECT
public:
    explicit AlpineFetcher(QObject *parent = nullptr);
    virtual ~AlpineFetcher();

    void fetch(const QUrl &url) override;

private slots:
    void onDownloadFinished(QNetworkReply *reply);

private:
    void parseIndex(const QByteArray &data);
};

} // namespace fetchers
} // namespace compositor
} // namespace uli

#endif // ULI_COMPOSITOR_ALPINE_FETCHER_HPP
