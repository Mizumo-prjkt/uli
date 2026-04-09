#include "RepoFetcher.hpp"

namespace uli {
namespace compositor {
namespace fetchers {

RepoFetcher::RepoFetcher(QObject *parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
}

RepoFetcher::~RepoFetcher() {}

} // namespace fetchers
} // namespace compositor
} // namespace uli
