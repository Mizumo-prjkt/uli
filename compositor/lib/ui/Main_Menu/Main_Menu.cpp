#include "Main_Menu.hpp"
#include <QApplication>
#include <QScreen>
#include <QStyle>
#include <QMessageBox>
#include <QFileDialog>
#include "../../components/yaml/YamlGenerator.hpp"
#include "../ProfileEditor/ProfileEditor.hpp"

namespace uli {
namespace compositor {
namespace ui {

Main_Menu::Main_Menu(QWidget *parent) : QMainWindow(parent) {
    syncManager = new SyncManager(this);
    
    connect(syncManager, &SyncManager::logMessage, this, &Main_Menu::appendLog);
    connect(syncManager, &SyncManager::progressUpdated, this, [this](int p){ progressBar->setValue(p); });
    connect(syncManager, &SyncManager::syncFinished, this, &Main_Menu::onSyncFinished);

    setupUi();
    applyStyles();
    
    // Center the window on the screen
    resize(1000, 700);
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);
    
    setWindowTitle("ULI Compositor - Unified Engine");
}

Main_Menu::~Main_Menu() {}

void Main_Menu::setupUi() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(25);
    
    // Header
    QLabel *headerLabel = new QLabel("ULI Compositor", this);
    headerLabel->setObjectName("headerLabel");
    mainLayout->addWidget(headerLabel);
    
    statusLabel = new QLabel("System Ready", this);
    statusLabel->setObjectName("statusLabel");
    mainLayout->addWidget(statusLabel);
    
    // Buttons Tray
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(20);
    
    btnSync = new QPushButton("Sync Repositories", this);
    btnEditor = new QPushButton("Profile Editor", this);
    btnOpenExisting = new QPushButton("Open Existing Profile", this);
    btnExport = new QPushButton("Export Composite YAML", this);
    
    buttonLayout->addWidget(btnSync);
    buttonLayout->addWidget(btnEditor);
    buttonLayout->addWidget(btnOpenExisting);
    buttonLayout->addWidget(btnExport);
    mainLayout->addLayout(buttonLayout);
    
    // Progress
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->hide();
    mainLayout->addWidget(progressBar);
    
    // Logs
    QLabel *logHeader = new QLabel("Process Logs", this);
    logHeader->setObjectName("logHeader");
    mainLayout->addWidget(logHeader);
    
    logView = new QListWidget(this);
    logView->setObjectName("logView");
    mainLayout->addWidget(logView);
    
    // Connections
    connect(btnSync, &QPushButton::clicked, this, &Main_Menu::onSyncClicked);
    connect(btnEditor, &QPushButton::clicked, this, &Main_Menu::onOpenEditorClicked);
    connect(btnOpenExisting, &QPushButton::clicked, this, &Main_Menu::onOpenExistingClicked);
    connect(btnExport, &QPushButton::clicked, this, &Main_Menu::onExportClicked);
}

void Main_Menu::applyStyles() {
    this->setStyleSheet(R"(
        QMainWindow {
            background-color: #0F0F13;
        }
        QWidget {
            color: #E0E0E0;
            font-family: 'Inter', 'Segoe UI', sans-serif;
        }
        #headerLabel {
            font-size: 32px;
            font-weight: 800;
            color: #FFFFFF;
            letter-spacing: 1px;
        }
        #statusLabel {
            font-size: 14px;
            color: #888899;
            margin-bottom: 10px;
        }
        #logHeader {
            font-size: 16px;
            font-weight: 600;
            color: #AAAABB;
            margin-top: 10px;
        }
        QPushButton {
            background-color: #1A1A24;
            border: 1px solid #2D2D3D;
            border-radius: 8px;
            padding: 12px 24px;
            font-size: 14px;
            font-weight: 600;
            min-width: 150px;
        }
        QPushButton:hover {
            background-color: #252535;
            border-color: #3D3D4D;
        }
        QPushButton:pressed {
            background-color: #12121A;
        }
        #logView {
            background-color: #12121A;
            border: 1px solid #252532;
            border-radius: 12px;
            padding: 10px;
            font-family: 'Cascadia Code', 'Consolas', monospace;
            font-size: 13px;
        }
        QProgressBar {
            background-color: #1A1A24;
            border: 1px solid #2D2D3D;
            border-radius: 10px;
            text-align: center;
            height: 20px;
            color: transparent;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4FACFE, stop:1 #00F2FE);
            border-radius: 8px;
        }
    )");
}

void Main_Menu::onSyncClicked() {
    logView->addItem("[INFO] Initiating repository synchronization...");
    
    btnSync->setEnabled(false);
    btnExport->setEnabled(false);
    
    progressBar->show();
    progressBar->setValue(0);
    statusLabel->setText("Status: Syncing Repositories...");
    
    syncManager->startSync();
}

void Main_Menu::onOpenEditorClicked() {
    logView->addItem("[UI] Launching Full-Featured Profile Editor...");
    ProfileEditor editor(syncManager, this);
    if (editor.exec() == QDialog::Accepted) {
        logView->addItem("[UI] Spec finalized and pending export.");
    }
}

void Main_Menu::onOpenExistingClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Existing Profile", "", "YAML Files (*.yaml)");
    if (fileName.isEmpty()) return;

    logView->addItem("[UI] Opening existing profile: " + fileName);
    ProfileEditor editor(syncManager, this);
    editor.loadProfile(fileName);
    if (editor.exec() == QDialog::Accepted) {
        logView->addItem("[UI] Edited profile saved/accepted.");
    }
}

void Main_Menu::onExportClicked() {
    if (currentComponents.isEmpty()) {
        QMessageBox::warning(this, "Export Error", "No synchronized components available to export. Please run 'Sync Repositories' first.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save Composite Profile", "composite_profile.yaml", "YAML Files (*.yaml)");
    if (fileName.isEmpty()) return;

    if (uli::compositor::yaml::YamlGenerator::saveToYaml(fileName, currentComponents)) {
        appendLog("[SUCCESS] Composite YAML exported to: " + fileName);
        QMessageBox::information(this, "Export Success", "Configuration saved successfully.");
    } else {
        appendLog("[ERROR] Failed to write YAML file.");
    }
}

void Main_Menu::onSyncFinished(const QList<categorization::CompositeComponent> &components) {
    currentComponents = components;
    btnSync->setEnabled(true);
    btnExport->setEnabled(true);
    statusLabel->setText("Status: Sync Complete. " + QString::number(components.size()) + " components mapped.");
}

void Main_Menu::appendLog(const QString &msg) {
    logView->addItem(msg);
    logView->scrollToBottom();
}

} // namespace ui
} // namespace compositor
} // namespace uli
