#ifndef ULI_COMPOSITOR_PARTITION_DIALOG_HPP
#define ULI_COMPOSITOR_PARTITION_DIALOG_HPP

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include "storage/StorageModel.hpp"

namespace uli {
namespace compositor {
namespace ui {

class PartitionDialog : public QDialog {
    Q_OBJECT
public:
    explicit PartitionDialog(storage::StorageModel *model, QWidget *parent = nullptr);
    storage::PartitionConfig config() const;
    void setConfig(const storage::PartitionConfig &cfg);

private:
    QLineEdit *editSize;
    QComboBox *comboFs;
    QLineEdit *editMount;
    QLineEdit *editLabel;
    storage::StorageModel *m_model;
};

} // namespace ui
} // namespace compositor
} // namespace uli

#endif // ULI_COMPOSITOR_PARTITION_DIALOG_HPP
