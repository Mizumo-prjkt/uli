#ifndef ULI_COMPOSITOR_USER_DIALOG_HPP
#define ULI_COMPOSITOR_USER_DIALOG_HPP

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>

namespace uli {
namespace compositor {
namespace ui {

struct UserSpec {
    QString username;
    QString password;
    bool isSudoer;
};

class UserDialog : public QDialog {
    Q_OBJECT
public:
    explicit UserDialog(QWidget *parent = nullptr);
    
    UserSpec spec() const;

private:
    QLineEdit *editUsername;
    QLineEdit *editPassword;
    QCheckBox *checkSudo;
};

} // namespace ui
} // namespace compositor
} // namespace uli

#endif // ULI_COMPOSITOR_USER_DIALOG_HPP
