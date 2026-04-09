#include "UserDialog.hpp"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QPushButton>

namespace uli {
namespace compositor {
namespace ui {

UserDialog::UserDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Setup User Account");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();
    
    editUsername = new QLineEdit(this);
    editPassword = new QLineEdit(this);
    editPassword->setEchoMode(QLineEdit::Password);
    checkSudo = new QCheckBox("Grant Administrative (Sudo) Privileges", this);
    checkSudo->setChecked(true);
    
    formLayout->addRow("Username:", editUsername);
    formLayout->addRow("Password:", editPassword);
    formLayout->addRow("", checkSudo);
    
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
                       "QLineEdit { background-color: #2D2D3D; border: 1px solid #444; color: white; border-radius: 4px; padding: 4px; }");
}

UserSpec UserDialog::spec() const {
    return {editUsername->text(), editPassword->text(), checkSudo->isChecked()};
}

} // namespace ui
} // namespace compositor
} // namespace uli
