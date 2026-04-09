#ifndef ULI_COMPOSITOR_MAIN_MENU_HPP
#define ULI_COMPOSITOR_MAIN_MENU_HPP

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QProgressBar>

#include "components/SyncManager.hpp"
#include "components/categorization/lib/PackageMapper.hpp"

namespace uli {
namespace compositor {
namespace ui {

class Main_Menu : public QMainWindow {
    Q_OBJECT
public:
    explicit Main_Menu(QWidget *parent = nullptr);
    virtual ~Main_Menu();

private slots:
    void onSyncClicked();
    void onOpenEditorClicked();
    void onOpenExistingClicked();
    void onExportClicked();
    
    void onSyncFinished(const QList<categorization::CompositeComponent> &components);
    void appendLog(const QString &msg);

private:
    void setupUi();
    void applyStyles();

    QPushButton *btnSync;
    QPushButton *btnOpenExisting;
    QPushButton *btnEditor;
    QPushButton *btnExport;
    QProgressBar *progressBar;
    QListWidget *logView;
    QLabel *statusLabel;
    
    SyncManager *syncManager;
    QList<categorization::CompositeComponent> currentComponents;
};

} // namespace ui
} // namespace compositor
} // namespace uli

#endif // ULI_COMPOSITOR_MAIN_MENU_HPP
