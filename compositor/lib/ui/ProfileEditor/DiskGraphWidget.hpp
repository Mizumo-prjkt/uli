#ifndef ULI_COMPOSITOR_DISK_GRAPH_WIDGET_HPP
#define ULI_COMPOSITOR_DISK_GRAPH_WIDGET_HPP

#include <QWidget>
#include "storage/StorageModel.hpp"

namespace uli {
namespace compositor {
namespace ui {

class DiskGraphWidget : public QWidget {
    Q_OBJECT
public:
    explicit DiskGraphWidget(storage::StorageModel *model, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;

private:
    QString getPartitionInfoAt(int x);
    storage::StorageModel *m_model;
};

} // namespace ui
} // namespace compositor
} // namespace uli

#endif // ULI_COMPOSITOR_DISK_GRAPH_WIDGET_HPP
