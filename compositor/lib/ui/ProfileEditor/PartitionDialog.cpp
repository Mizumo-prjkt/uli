#include "PartitionDialog.hpp"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QPushButton>

namespace uli {
namespace compositor {
namespace ui {

PartitionDialog::PartitionDialog(storage::StorageModel *model, QWidget *parent) 
    : QDialog(parent), m_model(model) {
    setWindowTitle("Define Virtual Partition");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();
    
    editSize = new QLineEdit(this);
    editSize->setPlaceholderText("e.g. +512M, +20G, or 0 for remaining space");
    
    comboFs = new QComboBox(this);
    comboFs->addItems({"ext4", "btrfs", "vfat", "xfs", "swap"});
    
    editMount = new QLineEdit(this);
    editLabel = new QLineEdit(this);
    
    formLayout->addRow("Partition Size:", editSize);
    formLayout->addRow("Filesystem:", comboFs);
    formLayout->addRow("Mount Point:", editMount);
    formLayout->addRow("Label:", editLabel);
    
    mainLayout->addLayout(formLayout);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnOk = new QPushButton("Apply", this);
    QPushButton *btnCancel = new QPushButton("Cancel", this);
    
    btnLayout->addStretch();
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnOk);
    mainLayout->addLayout(btnLayout);
    
    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    this->setStyleSheet("QDialog { background-color: #1A1A24; color: #FFFFFF; } "
                       "QLabel { color: #E0E0E0; } "
                       "QLineEdit, QComboBox { background-color: #2D2D3D; border: 1px solid #444; color: white; border-radius: 4px; padding: 4px; }");
}

storage::PartitionConfig PartitionDialog::config() const {
    storage::PartitionConfig cfg;
    cfg.size_cmd = editSize->text().isEmpty() ? "0" : editSize->text();
    cfg.fs_type = comboFs->currentText();
    cfg.mount_point = editMount->text();
    cfg.label = editLabel->text();
    cfg.device = "/dev/sda"; // Default theoretical device
    
    if (cfg.fs_type == "vfat") cfg.type_code = "ef00";
    else if (cfg.fs_type == "swap") cfg.type_code = "8200";
    else cfg.type_code = "8300";
    
    return cfg;
}

void PartitionDialog::setConfig(const storage::PartitionConfig &cfg) {
    editSize->setText(cfg.size_cmd);
    comboFs->setCurrentText(cfg.fs_type);
    editMount->setText(cfg.mount_point);
    editLabel->setText(cfg.label);
}

} // namespace ui
} // namespace compositor
} // namespace uli
