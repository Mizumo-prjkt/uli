#ifndef ULI_COMPOSITOR_ARCH_FETCHER_HPP
#define ULI_COMPOSITOR_ARCH_FETCHER_HPP

#include "../RepoFetcher.hpp"
#include <archive.h>
#include <archive_entry.h>

namespace uli {
namespace compositor {
namespace fetchers {

class ArchFetcher : public RepoFetcher {
    Q_OBJECT
public:
    explicit ArchFetcher(QObject *parent = nullptr);
    virtual ~ArchFetcher();

    void fetch(const QUrl &url) override;

private slots:
    void onDownloadFinished(QNetworkReply *reply);

private:
    void parseDatabase(const QByteArray &data);
};

} // namespace fetchers
} // namespace compositor
} // namespace uli

#endif // ULI_COMPOSITOR_ARCH_FETCHER_HPP
