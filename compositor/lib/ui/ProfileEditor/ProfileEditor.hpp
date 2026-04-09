#ifndef ULI_COMPOSITOR_PROFILE_EDITOR_HPP
#define ULI_COMPOSITOR_PROFILE_EDITOR_HPP

#include "../../components/ProfileExporter.hpp"
#include "DiskGraphWidget.hpp"
#include "UserDialog.hpp"
#include "storage/StorageModel.hpp"
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QStringListModel>

namespace uli {
namespace compositor {
class SyncManager;
namespace ui {

class ProfileEditor : public QDialog {
  Q_OBJECT
public:
  explicit ProfileEditor(SyncManager *syncMgr, QWidget *parent = nullptr);
  virtual ~ProfileEditor();

  void loadProfile(const QString &path);

private slots:
  void onSaveClicked();
  void onAddUserClicked();
  void onAddPartitionClicked();
  void onTheoreticalSizeChanged(int size);
  void onAgnosticToggled(bool checked);
  void onSearchTextChanged(const QString &text);
  void onDistroChanged(int);
  void onEditPartitionClicked();
  void onRemovePartitionClicked();
  void onPartitionDoubleClicked(int row, int column);
  void onSearchItemDoubleClicked(QTreeWidgetItem *item, int column);
  void onRemovePackageClicked();
  void onSelectedPackageDoubleClicked(QListWidgetItem *item);
  void onTreeContextMenu(const QPoint &pos);
  void onFillSlot();
  void onAddMappingClicked();
  void onRemoveMappingClicked();
  void onMappingTableDoubleClicked(int row, int column);
  void onImportClicked();
  void onSaveAsClicked();
  void onFieldChanged(); // Triggered by any UI change

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void closeEvent(QCloseEvent *event) override;

private:
  void setupUi();
  void setupGeneralTab(QWidget *tab);
  void setupStorageTab(QWidget *tab);
  void setupNetworkTab(QWidget *tab);
  void setupUserTab(QWidget *tab);
  void setupBootloaderTab(QWidget *tab);
  void setupComponentsTab(QWidget *tab);
  void applyStyles();
  QComboBox *createSlotComboBox(int distroCol);
  int addMappingRow(const QString &generic, const QString &arch = "",
                    const QString &alpine = "", const QString &debian = "");

  void saveAutosave();
  void checkForAutosave();
  void clearAutosave();
  QString getBestAutosaveDir() const;
  QString summarizeProfile(const QString &path) const;
  void handleAutosaveRecovery(const QStringList &files);
  void ensureModelsPopulated(int distroCol);

  QTabWidget *tabWidget;

  // General
  QLineEdit *editHostname;
  QComboBox *comboLanguage;
  QComboBox *comboTimezone;
  QComboBox *comboInitSystem;

  // Network
  QComboBox *comboNetworkBackend;
  QLineEdit *editWifiSsid;
  QLineEdit *editWifiPassphrase;

  // Storage
  QSpinBox *spinDiskSize;
  QTableWidget *tablePartitions;
  QPushButton *btnAddPart;
  QPushButton *btnEditPart;
  QPushButton *btnRemovePart;
  DiskGraphWidget *diskGraph;
  storage::StorageModel *storageModel;
  QLabel *lblDiskSummary;
  void refreshPartitionTable();

  // Users
  QListWidget *listUsers;
  QLineEdit *editRootPassword;

  // Bootloader
  QComboBox *comboBootTarget;
  QComboBox *comboEfiPart;
  QLineEdit *editEfiDir;
  QLineEdit *editBootloaderId;
  QCheckBox *checkIsEfi;
  void updateEfiPartCombo();

  // Components
  QCheckBox *checkAgnostic;
  QComboBox *comboTargetDistro;
  QLineEdit *editSearch;
  QTreeWidget *treeSearchSync;
  QListWidget *listSelectedPkgs;
  QPushButton *btnRemovePkg;

  // Workbench
  QTableWidget *tableMappings;
  QPushButton *btnAddMapping;
  QPushButton *btnRemoveMapping;

  QList<UserSpec> m_users;
  QList<LinkedPackage> m_linkedPkgs;
  SyncManager *m_syncMgr;

  // Performance & State
  QStringList m_cachedArchPkgs;
  QStringList m_cachedAlpinePkgs;
  QStringList m_cachedDebianPkgs;
  QString m_currentAutosavePath;
  bool m_isDirty = false;
  bool m_loading = false;
  QStringListModel *m_modelArch = nullptr;
  QStringListModel *m_modelAlpine = nullptr;
  QStringListModel *m_modelDebian = nullptr;
};

} // namespace ui
} // namespace compositor
} // namespace uli

#endif // ULI_COMPOSITOR_PROFILE_EDITOR_HPP
