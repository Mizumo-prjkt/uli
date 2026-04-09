#include "../../components/SyncManager.hpp"
#include "ProfileEditor.hpp"
#include <QVBoxLayout>
#include "../../components/ProfileImporter.hpp"
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QHeaderView>
#include "UserDialog.hpp"
#include "PartitionDialog.hpp"
#include "ProfileExporter.hpp"
#include <QMenu>
#include <QTableWidget>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QTimer>
#include <QDir>
#include <QProgressDialog>
#include <QCoreApplication>
#include <algorithm>

namespace uli {
namespace compositor {
namespace ui {

ProfileEditor::ProfileEditor(SyncManager *syncMgr, QWidget *parent) 
    : QDialog(parent), m_syncMgr(syncMgr) {
    m_loading = true;
    storageModel = new storage::StorageModel(this);
    
    // Initialize shared models
    m_modelArch = new QStringListModel(this);
    m_modelAlpine = new QStringListModel(this);
    m_modelDebian = new QStringListModel(this);
    
    setupUi();
    applyStyles();
    
    resize(1200, 800);
    setWindowTitle("ULI Profile Editor - System Architecture Designer");
    
    // Calculate session path immediately at launch
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    m_currentAutosavePath = getBestAutosaveDir() + "/.uli_profile_" + timestamp + ".swp";
    
    m_loading = false;
    
    // Check for other sessions and start current one
    QTimer::singleShot(100, this, &ProfileEditor::checkForAutosave);
    saveAutosave();
}

ProfileEditor::~ProfileEditor() {
    // Models deleted by parent-child tree (passed 'this' in ctor)
}


void ProfileEditor::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    tabWidget = new QTabWidget(this);
    
    QWidget *tabGeneral = new QWidget();
    setupGeneralTab(tabGeneral);
    tabWidget->addTab(tabGeneral, "General");

    QWidget *tabStorage = new QWidget();
    setupStorageTab(tabStorage);
    tabWidget->addTab(tabStorage, "Storage");

    QWidget *tabNetwork = new QWidget();
    setupNetworkTab(tabNetwork);
    tabWidget->addTab(tabNetwork, "Network");

    QWidget *tabUsers = new QWidget();
    setupUserTab(tabUsers);
    tabWidget->addTab(tabUsers, "Users");

    QWidget *tabBoot = new QWidget();
    setupBootloaderTab(tabBoot);
    tabWidget->addTab(tabBoot, "Bootloader");

    QWidget *tabComp = new QWidget();
    setupComponentsTab(tabComp);
    tabWidget->addTab(tabComp, "Components");

    mainLayout->addWidget(tabWidget);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnCancel = new QPushButton("Discard Changes", this);
    QPushButton *btnImport = new QPushButton("Import YAML...", this);
    QPushButton *btnSaveAs = new QPushButton("Save Copy...", this);
    QPushButton *btnSave = new QPushButton("Generate Profile", this);
    btnSave->setObjectName("btnSave");
    
    btnLayout->addWidget(btnImport);
    btnLayout->addStretch();
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnSaveAs);
    btnLayout->addWidget(btnSave);
    mainLayout->addLayout(btnLayout);

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnImport, &QPushButton::clicked, this, &ProfileEditor::onImportClicked);
    connect(btnSaveAs, &QPushButton::clicked, this, &ProfileEditor::onSaveAsClicked);
    connect(btnSave, &QPushButton::clicked, this, &ProfileEditor::onSaveClicked);
}

void ProfileEditor::setupGeneralTab(QWidget *tab) {
    QFormLayout *layout = new QFormLayout(tab);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(15);

    editHostname = new QLineEdit("uli-host", tab);
    comboLanguage = new QComboBox(tab);
    comboLanguage->addItems({"English", "Filipino", "Spanish", "French", "German", "Italian"});
    
    comboTimezone = new QComboBox(tab);
    comboTimezone->addItems({"UTC", "Asia/Manila", "America/New_York", "Europe/London", "Asia/Tokyo"});

    comboInitSystem = new QComboBox(tab);
    comboInitSystem->addItems({"systemd", "OpenRC", "SysVinit"});

    layout->addRow("System Hostname:", editHostname);
    layout->addRow("Preferred Language:", comboLanguage);
    layout->addRow("System Timezone:", comboTimezone);
    layout->addRow("Init System (Service Manager):", comboInitSystem);

    connect(editHostname, &QLineEdit::textChanged, this, &ProfileEditor::onFieldChanged);
    connect(comboLanguage, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProfileEditor::onFieldChanged);
    connect(comboTimezone, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProfileEditor::onFieldChanged);
    connect(comboInitSystem, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProfileEditor::onFieldChanged);
}

void ProfileEditor::setupStorageTab(QWidget *tab) {
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(10);

    // ── Top Bar: Disk Size Config (compact) ─────────────────────────────
    QWidget *diskBar = new QWidget(tab);
    diskBar->setObjectName("distroBar");  // Reuse the same styled bar
    QHBoxLayout *diskBarLayout = new QHBoxLayout(diskBar);
    diskBarLayout->setContentsMargins(12, 8, 12, 8);
    diskBarLayout->setSpacing(12);

    QLabel *lblDiskIcon = new QLabel("💾", diskBar);
    lblDiskIcon->setStyleSheet("font-size: 18px;");

    QLabel *lblDiskTitle = new QLabel("Virtual Disk Configuration", diskBar);
    lblDiskTitle->setObjectName("sectionLabel");

    QLabel *lblSizeLabel = new QLabel("Theoretical Size:", diskBar);
    spinDiskSize = new QSpinBox(diskBar);
    spinDiskSize->setRange(14, 100000);
    spinDiskSize->setValue(500);
    spinDiskSize->setSuffix(" GB");
    spinDiskSize->setMinimumWidth(120);

    diskBarLayout->addWidget(lblDiskIcon);
    diskBarLayout->addWidget(lblDiskTitle);
    diskBarLayout->addStretch();
    diskBarLayout->addWidget(lblSizeLabel);
    diskBarLayout->addWidget(spinDiskSize);
    layout->addWidget(diskBar);

    // ── Disk Visualization ──────────────────────────────────────────────
    QLabel *lblVizTitle = new QLabel("Disk Occupancy", tab);
    lblVizTitle->setObjectName("sectionLabel");
    layout->addWidget(lblVizTitle);

    diskGraph = new DiskGraphWidget(storageModel, tab);
    diskGraph->setFixedHeight(80);
    layout->addWidget(diskGraph);

    // Summary line showing used / free
    lblDiskSummary = new QLabel(tab);
    lblDiskSummary->setObjectName("hintLabel");
    lblDiskSummary->setText("No partitions defined — entire disk is unallocated");
    layout->addWidget(lblDiskSummary);

    // ── FS Legend (inline) ──────────────────────────────────────────────
    QWidget *legendBar = new QWidget(tab);
    QHBoxLayout *legendLayout = new QHBoxLayout(legendBar);
    legendLayout->setContentsMargins(0, 0, 0, 0);
    legendLayout->setSpacing(16);

    auto addLegendItem = [&](const QString &color, const QString &label) {
        QWidget *dot = new QWidget(legendBar);
        dot->setFixedSize(10, 10);
        dot->setStyleSheet(QString("background-color: %1; border-radius: 5px;").arg(color));
        QLabel *lbl = new QLabel(label, legendBar);
        lbl->setStyleSheet("color: #888899; font-size: 11px;");
        legendLayout->addWidget(dot);
        legendLayout->addWidget(lbl);
    };
    addLegendItem("#00F2FE", "FAT32/vfat");
    addLegendItem("#4FACFE", "ext4");
    addLegendItem("#00cdac", "btrfs");
    addLegendItem("#667eea", "swap");
    addLegendItem("#888899", "Other");
    addLegendItem("#252532", "Unallocated");
    legendLayout->addStretch();
    layout->addWidget(legendBar);

    // ── Partition Table ─────────────────────────────────────────────────
    QLabel *lblPartTitle = new QLabel("Partition Layout", tab);
    lblPartTitle->setObjectName("sectionLabel");
    layout->addWidget(lblPartTitle);

    tablePartitions = new QTableWidget(0, 5, tab);
    tablePartitions->setHorizontalHeaderLabels({"#", "Size", "Filesystem", "Mount Point", "Label"});
    tablePartitions->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    tablePartitions->horizontalHeader()->resizeSection(0, 40);
    tablePartitions->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    tablePartitions->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    tablePartitions->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    tablePartitions->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    tablePartitions->verticalHeader()->setVisible(false);
    tablePartitions->setSelectionBehavior(QAbstractItemView::SelectRows);
    tablePartitions->setSelectionMode(QAbstractItemView::SingleSelection);
    tablePartitions->setAlternatingRowColors(true);
    tablePartitions->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tablePartitions->setStyleSheet(
        "QTableWidget { background-color: #12121A; alternate-background-color: #16161F; "
        "selection-background-color: #2D2D3D; border: 1px solid #252532; border-radius: 6px; "
        "color: #CCCCCC; gridline-color: #252532; }"
    );
    layout->addWidget(tablePartitions, 1);

    // ── Action Buttons ──────────────────────────────────────────────────
    QHBoxLayout *partBtnLayout = new QHBoxLayout();
    partBtnLayout->setSpacing(8);

    btnAddPart = new QPushButton("+ Add Partition", tab);
    btnAddPart->setObjectName("btnAccent");
    btnEditPart = new QPushButton("Edit Selected", tab);
    btnEditPart->setEnabled(false);
    btnRemovePart = new QPushButton("Remove Selected", tab);
    btnRemovePart->setObjectName("btnDanger");
    btnRemovePart->setEnabled(false);

    partBtnLayout->addWidget(btnAddPart);
    partBtnLayout->addWidget(btnEditPart);
    partBtnLayout->addStretch();
    partBtnLayout->addWidget(btnRemovePart);
    layout->addLayout(partBtnLayout);

    // ── Connections ─────────────────────────────────────────────────────
    connect(spinDiskSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &ProfileEditor::onTheoreticalSizeChanged);
    connect(btnAddPart, &QPushButton::clicked, this, &ProfileEditor::onAddPartitionClicked);
    connect(btnEditPart, &QPushButton::clicked, this, &ProfileEditor::onEditPartitionClicked);
    connect(btnRemovePart, &QPushButton::clicked, this, &ProfileEditor::onRemovePartitionClicked);
    connect(tablePartitions, &QTableWidget::cellDoubleClicked, this, &ProfileEditor::onPartitionDoubleClicked);
    connect(tablePartitions, &QTableWidget::itemSelectionChanged, this, [this](){
        bool selected = !tablePartitions->selectedItems().isEmpty();
        btnEditPart->setEnabled(selected);
        btnRemovePart->setEnabled(selected);
    });

    // Update summary whenever model changes
    connect(storageModel, &storage::StorageModel::modelChanged, this, [this]() {
        const auto &parts = storageModel->partitions();
        if (parts.isEmpty()) {
            lblDiskSummary->setText("No partitions defined — entire disk is unallocated");
        } else {
            long long total = storageModel->totalBytes();
            double totalGb = static_cast<double>(total) / (1024.0*1024*1024);
            
            // Calculate explicit usage separately
            long long explicitUsed = 0;
            bool hasRemainder = false;
            for (const auto &p : parts) {
                QString clean = p.size_cmd.trimmed();
                if (clean.startsWith('+')) clean = clean.mid(1);
                if (clean == "0") {
                    hasRemainder = true;
                } else {
                    explicitUsed += storage::StorageModel::parseSizeCmdToBytes(p.size_cmd, total);
                }
            }
            
            double explicitGb = static_cast<double>(explicitUsed) / (1024.0*1024*1024);
            double freeGb = totalGb - explicitGb;

            if (hasRemainder) {
                lblDiskSummary->setText(QString("%1 partition(s) • %2 GB allocated • remainder fills %3 GB")
                    .arg(parts.size())
                    .arg(explicitGb, 0, 'f', 1)
                    .arg(freeGb < 0 ? 0 : freeGb, 0, 'f', 1));
            } else {
                lblDiskSummary->setText(QString("%1 partition(s) • %2 GB allocated • %3 GB free")
                    .arg(parts.size())
                    .arg(explicitGb, 0, 'f', 1)
                    .arg(freeGb < 0 ? 0 : freeGb, 0, 'f', 1));
            }
        }
    });

}


void ProfileEditor::setupNetworkTab(QWidget *tab) {
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(30,30,30,30);
    layout->setSpacing(20);

    // Backend Selection
    QGroupBox *groupBackend = new QGroupBox("Primary Network Stack", tab);
    QFormLayout *formBackend = new QFormLayout(groupBackend);
    comboNetworkBackend = new QComboBox(groupBackend);
    comboNetworkBackend->addItems({"NetworkManager", "systemd-networkd", "dhcpcd", "iwd", "None (Manual)"});
    formBackend->addRow("Network Backend:", comboNetworkBackend);

    // Wireless Credentials
    QGroupBox *groupWireless = new QGroupBox("Wireless Credentials (Optional)", tab);
    QFormLayout *formWireless = new QFormLayout(groupWireless);
    editWifiSsid = new QLineEdit(groupWireless);
    editWifiSsid->setPlaceholderText("SSID (Example: MyHomeWiFi)");
    
    editWifiPassphrase = new QLineEdit(groupWireless);
    editWifiPassphrase->setEchoMode(QLineEdit::Password);
    editWifiPassphrase->setPlaceholderText("Passphrase (WPA/WPA2)");

    formWireless->addRow("SSID:", editWifiSsid);
    formWireless->addRow("Passphrase:", editWifiPassphrase);

    layout->addWidget(groupBackend);
    layout->addWidget(groupWireless);
    layout->addStretch();

    connect(comboNetworkBackend, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProfileEditor::onFieldChanged);
    connect(editWifiSsid, &QLineEdit::textChanged, this, &ProfileEditor::onFieldChanged);
    connect(editWifiPassphrase, &QLineEdit::textChanged, this, &ProfileEditor::onFieldChanged);
}

void ProfileEditor::setupUserTab(QWidget *tab) {
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(30, 30, 30, 30);

    QFormLayout *rootLayout = new QFormLayout();
    editRootPassword = new QLineEdit(tab);
    editRootPassword->setEchoMode(QLineEdit::Password);
    rootLayout->addRow("Root Password:", editRootPassword);
    layout->addLayout(rootLayout);

    connect(editRootPassword, &QLineEdit::textChanged, this, &ProfileEditor::onFieldChanged);

    QGroupBox *userBox = new QGroupBox("User Accounts", tab);
    QVBoxLayout *userLayout = new QVBoxLayout(userBox);
    listUsers = new QListWidget(userBox);
    QPushButton *btnAddUser = new QPushButton("Add New User", userBox);
    userLayout->addWidget(listUsers);
    userLayout->addWidget(btnAddUser);
    layout->addWidget(userBox);

    connect(btnAddUser, &QPushButton::clicked, this, &ProfileEditor::onAddUserClicked);
}

void ProfileEditor::setupBootloaderTab(QWidget *tab) {
    QFormLayout *layout = new QFormLayout(tab);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(15);

    comboBootTarget = new QComboBox(tab);
    comboBootTarget->addItems({"grub", "systemd-boot", "limine", "none"});
    
    checkIsEfi = new QCheckBox("Interface is UEFI (Firmware condition)", tab);
    checkIsEfi->setChecked(true);

    comboEfiPart = new QComboBox(tab);
    updateEfiPartCombo();

    editEfiDir = new QLineEdit(tab);
    editEfiDir->setPlaceholderText("/boot");

    editBootloaderId = new QLineEdit(tab);
    editBootloaderId->setPlaceholderText("ULI OS");

    layout->addRow("Bootloader Type:", comboBootTarget);
    layout->addRow("Hardware Check:", checkIsEfi);
    layout->addRow("EFI System Partition (ESP):", comboEfiPart);
    layout->addRow("EFI Directory (Mount Path):", editEfiDir);
    layout->addRow("Bootloader ID:", editBootloaderId);

    connect(comboBootTarget, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProfileEditor::onFieldChanged);
    connect(checkIsEfi, &QCheckBox::toggled, this, &ProfileEditor::onFieldChanged);
    connect(editEfiDir, &QLineEdit::textChanged, this, &ProfileEditor::onFieldChanged);
    connect(editBootloaderId, &QLineEdit::textChanged, this, &ProfileEditor::onFieldChanged);

    // Auto-fill EFI Dir when ESP selection changes
    connect(comboEfiPart, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index < 0) return;
        QVariant data = comboEfiPart->itemData(index);
        if (data.isValid()) {
            QString mount = data.toString();
            if (!mount.isEmpty() && mount != "—") {
                editEfiDir->setText(mount);
            }
        }
    });

    // Update available partitions whenever Storage model changes
    connect(storageModel, &storage::StorageModel::modelChanged, this, &ProfileEditor::updateEfiPartCombo);
}

void ProfileEditor::updateEfiPartCombo() {
    if (!comboEfiPart) return;
    
    // Save current selection if any
    int currentPartNum = comboEfiPart->currentText().split(' ').last().toInt();
    
    comboEfiPart->clear();
    const auto &parts = storageModel->partitions();
    
    int selectIdx = -1;
    for (int i = 0; i < parts.size(); ++i) {
        const auto &p = parts[i];
        QString label = QString("Partition #%1 (%2, %3)")
            .arg(p.part_num)
            .arg(p.fs_type)
            .arg(p.size_cmd);
        
        // Store mount point in item data for auto-fill logic
        comboEfiPart->addItem(label, p.mount_point);
        
        if (p.part_num == currentPartNum) {
            selectIdx = i;
        }
    }
    
    if (selectIdx != -1) {
        comboEfiPart->setCurrentIndex(selectIdx);
    }
}


void ProfileEditor::setupComponentsTab(QWidget *tab) {
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(8);

    // ── Top Bar: Distribution Config (compact, horizontal) ──────────────
    QWidget *distroBar = new QWidget(tab);
    distroBar->setObjectName("distroBar");
    QHBoxLayout *distroLayout = new QHBoxLayout(distroBar);
    distroLayout->setContentsMargins(12, 8, 12, 8);
    distroLayout->setSpacing(12);

    QLabel *lblBaseplate = new QLabel("Design Baseplate:", distroBar);
    comboTargetDistro = new QComboBox(distroBar);
    comboTargetDistro->addItems({"Arch Linux", "Alpine Linux", "Debian"});
    comboTargetDistro->setMinimumWidth(160);

    checkAgnostic = new QCheckBox("Distro-Agnostic Mode", distroBar);
    checkAgnostic->setToolTip("Enable Cross-Repo Resolution across all distributions");

    distroLayout->addWidget(lblBaseplate);
    distroLayout->addWidget(comboTargetDistro);
    distroLayout->addSpacing(20);
    distroLayout->addWidget(checkAgnostic);
    distroLayout->addStretch();
    layout->addWidget(distroBar);

    // ── Main Content: Horizontal Splitter ────────────────────────────────
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal, tab);
    mainSplitter->setHandleWidth(3);
    mainSplitter->setChildrenCollapsible(false);

    // ═══════════════════════════════════════════════════════════════════
    //  LEFT PANEL: Package Search
    // ═══════════════════════════════════════════════════════════════════
    QWidget *leftPanel = new QWidget(mainSplitter);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 4, 0);
    leftLayout->setSpacing(6);

    QLabel *lblSearch = new QLabel("Multi-Distribution Package Search", leftPanel);
    lblSearch->setObjectName("sectionLabel");

    editSearch = new QLineEdit(leftPanel);
    editSearch->setObjectName("searchField");
    editSearch->setPlaceholderText("Search packages across Arch, Alpine, Debian...");
    editSearch->setClearButtonEnabled(true);

    treeSearchSync = new QTreeWidget(leftPanel);
    treeSearchSync->setHeaderLabels({"#", "Arch Linux", "Alpine Linux", "Debian"});
    treeSearchSync->header()->setSectionResizeMode(QHeaderView::Interactive);
    treeSearchSync->header()->resizeSection(0, 40);
    treeSearchSync->header()->resizeSection(1, 180);
    treeSearchSync->header()->resizeSection(2, 180);
    treeSearchSync->header()->resizeSection(3, 180);
    treeSearchSync->setAlternatingRowColors(true);
    treeSearchSync->setRootIsDecorated(false);
    treeSearchSync->setMinimumHeight(200);

    QLabel *lblSearchHint = new QLabel("Double-click a cell to add • Right-click for slot filling", leftPanel);
    lblSearchHint->setObjectName("hintLabel");

    leftLayout->addWidget(lblSearch);
    leftLayout->addWidget(editSearch);
    leftLayout->addWidget(treeSearchSync, 1);
    leftLayout->addWidget(lblSearchHint);

    // ═══════════════════════════════════════════════════════════════════
    //  RIGHT PANEL: Selected Packages + Workbench (stacked vertically)
    // ═══════════════════════════════════════════════════════════════════
    QWidget *rightPanel = new QWidget(mainSplitter);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 0, 0, 0);
    rightLayout->setSpacing(6);

    // ─── Selected Packages Section ───────────────────────────────────
    QLabel *lblSelected = new QLabel("Selected Profile Packages", rightPanel);
    lblSelected->setObjectName("sectionLabel");

    listSelectedPkgs = new QListWidget(rightPanel);
    listSelectedPkgs->setMinimumHeight(120);
    listSelectedPkgs->setAlternatingRowColors(true);

    btnRemovePkg = new QPushButton("Remove Selected", rightPanel);
    btnRemovePkg->setEnabled(false);
    btnRemovePkg->setObjectName("btnDanger");

    rightLayout->addWidget(lblSelected);
    rightLayout->addWidget(listSelectedPkgs, 1);
    rightLayout->addWidget(btnRemovePkg);

    // ─── Linked Component Workbench ──────────────────────────────────
    QLabel *lblWorkbench = new QLabel("Linked Component Workbench", rightPanel);
    lblWorkbench->setObjectName("sectionLabel");

    QLabel *lblWorkbenchHint = new QLabel(
        "Map generic names to distro-specific packages. "
        "Right-click search results → Fill Slot.", rightPanel);
    lblWorkbenchHint->setObjectName("hintLabel");
    lblWorkbenchHint->setWordWrap(true);

    tableMappings = new QTableWidget(0, 4, rightPanel);
    tableMappings->setHorizontalHeaderLabels({"Generic Name", "Arch", "Alpine", "Debian"});
    tableMappings->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableMappings->setAlternatingRowColors(true);
    tableMappings->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableMappings->setMinimumHeight(100);
    tableMappings->verticalHeader()->setVisible(false);

    QHBoxLayout *workBtnLayout = new QHBoxLayout();
    workBtnLayout->setSpacing(6);
    btnAddMapping = new QPushButton("+ New Mapping", rightPanel);
    btnAddMapping->setObjectName("btnAccent");
    btnRemoveMapping = new QPushButton("Remove Mapping", rightPanel);
    btnRemoveMapping->setObjectName("btnDanger");
    workBtnLayout->addWidget(btnAddMapping);
    workBtnLayout->addWidget(btnRemoveMapping);
    workBtnLayout->addStretch();

    rightLayout->addSpacing(8);
    rightLayout->addWidget(lblWorkbench);
    rightLayout->addWidget(lblWorkbenchHint);
    rightLayout->addWidget(tableMappings, 1);
    rightLayout->addLayout(workBtnLayout);

    // ── Assemble splitter ────────────────────────────────────────────
    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setSizes({580, 420});
    layout->addWidget(mainSplitter, 1);

    // ── Connections ──────────────────────────────────────────────────
    treeSearchSync->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(checkAgnostic, &QCheckBox::toggled, this, &ProfileEditor::onAgnosticToggled);
    connect(editSearch, &QLineEdit::textChanged, this, &ProfileEditor::onSearchTextChanged);
    connect(comboTargetDistro, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProfileEditor::onDistroChanged);

    connect(checkAgnostic, &QCheckBox::toggled, this, &ProfileEditor::onFieldChanged);
    connect(comboTargetDistro, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ProfileEditor::onFieldChanged);
    connect(treeSearchSync, &QTreeWidget::itemDoubleClicked, this, &ProfileEditor::onSearchItemDoubleClicked);
    connect(treeSearchSync, &QTreeWidget::customContextMenuRequested, this, &ProfileEditor::onTreeContextMenu);
    connect(btnRemovePkg, &QPushButton::clicked, this, &ProfileEditor::onRemovePackageClicked);
    connect(listSelectedPkgs, &QListWidget::itemDoubleClicked, this, &ProfileEditor::onSelectedPackageDoubleClicked);
    connect(btnAddMapping, &QPushButton::clicked, this, &ProfileEditor::onAddMappingClicked);
    connect(btnRemoveMapping, &QPushButton::clicked, this, &ProfileEditor::onRemoveMappingClicked);
    connect(tableMappings, &QTableWidget::cellDoubleClicked, this, &ProfileEditor::onMappingTableDoubleClicked);
    connect(listSelectedPkgs, &QListWidget::itemSelectionChanged, this, [this](){
        btnRemovePkg->setEnabled(!listSelectedPkgs->selectedItems().isEmpty());
    });
}


void ProfileEditor::applyStyles() {
    this->setStyleSheet(R"(
        QDialog { background-color: #0F0F13; }
        QTabWidget::pane { border: 1px solid #2D2D3D; background-color: #15151E; border-radius: 8px; }
        QTabBar::tab { background-color: #1A1A24; color: #888899; padding: 10px 20px; border-top-left-radius: 8px; border-top-right-radius: 8px; margin-right: 2px; }
        QTabBar::tab:selected { background-color: #15151E; color: #FFFFFF; }
        QLabel { color: #E0E0E0; font-size: 13px; }
        QLineEdit, QComboBox, QSpinBox { background-color: #1F1F2E; border: 1px solid #353545; border-radius: 4px; padding: 6px; color: #FFFFFF; }
        QGroupBox { color: #4FACFE; font-weight: bold; border: 1px solid #2D2D3D; margin-top: 15px; padding-top: 20px; border-radius: 6px; }
        QPushButton { background-color: #1F1F2E; border: 1px solid #353545; border-radius: 6px; padding: 8px 16px; color: #FFFFFF; font-weight: 500; }
        QPushButton:hover { background-color: #2D2D3D; border-color: #4FACFE; }
        QPushButton:disabled { color: #555566; border-color: #252532; }
        #btnSave { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4FACFE, stop:1 #00F2FE); border: none; color: #000000; font-weight: bold; }
        QListWidget, QTreeWidget { background-color: #12121A; border: 1px solid #252532; border-radius: 6px; color: #CCCCCC; }
        QHeaderView::section { background-color: #1A1A24; color: #4FACFE; border: none; padding: 4px; font-weight: bold; }

        /* Components Tab: Section Labels */
        #sectionLabel { color: #4FACFE; font-size: 14px; font-weight: bold; padding: 2px 0; }
        #hintLabel { color: #555570; font-size: 11px; font-style: italic; padding: 2px 0; }

        /* Components Tab: Distro Config Bar */
        #distroBar { background-color: #1A1A24; border: 1px solid #2D2D3D; border-radius: 6px; }

        /* Components Tab: Search Field */
        #searchField { padding: 8px 12px; font-size: 13px; border-radius: 6px; }
        #searchField:focus { border-color: #4FACFE; }

        /* Components Tab: Accent & Danger Buttons */
        #btnAccent { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4FACFE, stop:1 #00F2FE); border: none; color: #000000; font-weight: bold; padding: 7px 14px; }
        #btnAccent:hover { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #6FBEFF, stop:1 #33F5FE); }
        #btnDanger { border-color: #8B3A3A; color: #E06060; }
        #btnDanger:hover { background-color: #3A1A1A; border-color: #E06060; }
        #btnDanger:disabled { color: #555566; border-color: #252532; background-color: #1F1F2E; }

        /* Components Tab: Splitter Handle */
        QSplitter::handle:horizontal { background-color: #2D2D3D; width: 3px; border-radius: 1px; }
        QSplitter::handle:horizontal:hover { background-color: #4FACFE; }

        /* Alternating row colors for lists & tables */
        QTreeWidget { alternate-background-color: #16161F; }
        QListWidget { alternate-background-color: #16161F; }
        QTableWidget { background-color: #12121A; alternate-background-color: #16161F; selection-background-color: #2D2D3D; border: 1px solid #252532; border-radius: 6px; color: #CCCCCC; gridline-color: #252532; }
        QTableWidget QHeaderView::section { background-color: #1A1A24; color: #4FACFE; border: none; padding: 6px; font-weight: bold; }

        QCheckBox { color: #CCCCCC; spacing: 6px; }
        QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #353545; border-radius: 3px; background-color: #1F1F2E; }
        QCheckBox::indicator:checked { background-color: #4FACFE; border-color: #4FACFE; }
    )");
}

QComboBox *ProfileEditor::createSlotComboBox(int distroCol) {
  QComboBox *combo = new QComboBox(tableMappings);
  combo->setEditable(true);
  combo->setInsertPolicy(QComboBox::NoInsert);
  combo->setMaxVisibleItems(12);
  combo->lineEdit()->setPlaceholderText(distroCol == 1   ? "Type to search Arch..."
                                        : distroCol == 2 ? "Type to search Alpine..."
                                                         : "Type to search Debian...");

  // PERFORMANCE FIX: Use shared models instead of copying 50k+ items every time
  ensureModelsPopulated(distroCol);
  QStringListModel *targetModel = (distroCol == 1)   ? m_modelArch
                                  : (distroCol == 2) ? m_modelAlpine
                                                     : m_modelDebian;

  combo->setModel(targetModel);

  // Attach a shared-source case-insensitive completer for fast filtering
  QCompleter *completer = new QCompleter(targetModel, combo);
  completer->setCaseSensitivity(Qt::CaseInsensitive);
  completer->setFilterMode(Qt::MatchContains);
  completer->setMaxVisibleItems(10);
  combo->setCompleter(completer);

  combo->setCurrentIndex(-1);
  combo->lineEdit()->clear();

  combo->setStyleSheet(
      "QComboBox { background-color: #1F1F2E; border: 1px solid #353545; "
      "border-radius: 3px; padding: 3px 6px; color: #FFFFFF; font-size: 12px; }"
      "QComboBox:focus { border-color: #4FACFE; }"
      "QComboBox QAbstractItemView { background-color: #1A1A24; color: #CCCCCC; "
      "selection-background-color: #2D2D3D; border: 1px solid #353545; }");

  return combo;
}

int ProfileEditor::addMappingRow(const QString &generic, const QString &arch, const QString &alpine, const QString &debian) {
    LinkedPackage lp;
    lp.genericName = generic;
    lp.archPkg = arch;
    lp.alpinePkg = alpine;
    lp.debianPkg = debian;
    m_linkedPkgs.append(lp);

    int row = tableMappings->rowCount();
    tableMappings->insertRow(row);

    // Column 0: Generic name (plain text item)
    tableMappings->setItem(row, 0, new QTableWidgetItem(generic));

    // Columns 1-3: Autocomplete comboboxes
    for (int col = 1; col <= 3; ++col) {
        QComboBox *combo = createSlotComboBox(col);
        QString preset = (col == 1) ? arch : (col == 2) ? alpine : debian;
        if (!preset.isEmpty()) {
            combo->setCurrentText(preset);
        }

        // Connect changes to update m_linkedPkgs
        connect(combo, &QComboBox::currentTextChanged, this, [this, row, col](const QString &text) {
            if (row < m_linkedPkgs.size()) {
                if (col == 1) m_linkedPkgs[row].archPkg = text;
                else if (col == 2) m_linkedPkgs[row].alpinePkg = text;
                else if (col == 3) m_linkedPkgs[row].debianPkg = text;
                onFieldChanged();
            }
        });

        tableMappings->setCellWidget(row, col, combo);
    }

    // Add to Selected Packages list with badge
    bool alreadyListed = false;
    for (int i = 0; i < listSelectedPkgs->count(); ++i) {
        if (listSelectedPkgs->item(i)->text().startsWith(generic)) {
            listSelectedPkgs->item(i)->setText(generic + " [Linked]");
            alreadyListed = true;
            break;
        }
    }
    if (!alreadyListed) {
        listSelectedPkgs->addItem(generic + " [Linked]");
    }

    tableMappings->selectRow(row);
    onFieldChanged();
    return row;
}

void ProfileEditor::onSaveClicked() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save Installer Profile", "uli_profile.yaml", "YAML Files (*.yaml)");
    if (fileName.isEmpty()) return;

    ProfileSpec spec;
    spec.hostname = editHostname->text();
    spec.language = comboLanguage->currentText();
    spec.timezone = comboTimezone->currentText();
    spec.rootPassword = editRootPassword->text();
    spec.bootTarget = comboBootTarget->currentText();
    spec.isEfi = checkIsEfi->isChecked();
    spec.isAgnostic = checkAgnostic->isChecked();
    spec.targetDistro = comboTargetDistro->currentText();
    spec.initSystem = comboInitSystem->currentText();
    spec.networkBackend = comboNetworkBackend->currentText();
    spec.wifiSsid = editWifiSsid->text();
    spec.wifiPassphrase = editWifiPassphrase->text();
    spec.users = m_users;
    spec.partitions = storageModel->partitions();
    
    spec.bootloaderId = editBootloaderId->text();
    spec.efiDirectory = editEfiDir->text();
    
    // Extract partition number from "Partition #X ..."
    QString efiPartText = comboEfiPart->currentText();
    if (efiPartText.contains('#')) {
        // e.g., "Partition #1 (vfat, 200MiB)"
        spec.efiPart = efiPartText.split('#')[1].split(' ')[0].toInt();
    } else {
        spec.efiPart = 0;
    }

    
    for (int i=0; i < listSelectedPkgs->count(); i++) {
        spec.additionalPkgs << listSelectedPkgs->item(i)->text().remove(" [Linked]");
    }
    spec.linkedPkgs = m_linkedPkgs;

    if (uli::compositor::ProfileExporter::exportToYaml(fileName, spec)) {
        QMessageBox::information(this, "Export Success", "Specification saved to: " + fileName);
        clearAutosave();
        m_isDirty = false;
        accept();
    } else {
        QMessageBox::critical(this, "Export Failure", "Could not write to the target specification file.");
    }
}

void ProfileEditor::loadProfile(const QString &path) {
  qDebug() << "[Loader] Attempting to import YAML from:" << path;
  ProfileSpec spec;
  if (!ProfileImporter::importFromYaml(path, spec)) {
    qWarning() << "[Loader] Import failed for:" << path;
    return;
  }

  qDebug() << "[Loader] Syncing UI state...";
  m_loading = true; // Block triggers during load
  tableMappings->setUpdatesEnabled(false);

  if (editHostname) editHostname->setText(spec.hostname);
  if (comboLanguage) comboLanguage->setCurrentText(spec.language);
  if (comboTimezone) comboTimezone->setCurrentText(spec.timezone);
  if (editRootPassword) editRootPassword->setText(spec.rootPassword);
  if (comboBootTarget) comboBootTarget->setCurrentText(spec.bootTarget);
  if (checkIsEfi) checkIsEfi->setChecked(spec.isEfi);
  if (checkAgnostic) checkAgnostic->setChecked(spec.isAgnostic);
  if (comboTargetDistro) comboTargetDistro->setCurrentText(spec.targetDistro);
  if (comboInitSystem) comboInitSystem->setCurrentText(spec.initSystem);
  
  if (comboNetworkBackend) comboNetworkBackend->setCurrentText(spec.networkBackend);
  if (editWifiSsid) editWifiSsid->setText(spec.wifiSsid);
  if (editWifiPassphrase) editWifiPassphrase->setText(spec.wifiPassphrase);

  // Bootloader
  if (editBootloaderId) editBootloaderId->setText(spec.bootloaderId);
  if (editEfiDir) editEfiDir->setText(spec.efiDirectory);

  // Users
  m_users = spec.users;
  listUsers->clear();
  for (const auto &u : m_users) {
    listUsers->addItem(u.username + (u.isSudoer ? " (Sudoer)" : ""));
  }

  // Partitions
  storageModel->setPartitions(spec.partitions);
  refreshPartitionTable();

  // Select correct ESP partition
  for (int i = 0; i < comboEfiPart->count(); ++i) {
    if (comboEfiPart->itemText(i).contains(QString("#%1 ").arg(spec.efiPart))) {
      comboEfiPart->setCurrentIndex(i);
      break;
    }
  }

  // Packages
  listSelectedPkgs->clear();
  for (const auto &pkg : spec.additionalPkgs) {
    listSelectedPkgs->addItem(pkg);
  }

  // Linked Packages (Workbench) — use addMappingRow for combobox widgets
  QList<LinkedPackage> importedLinks = spec.linkedPkgs;
  m_linkedPkgs.clear();
  tableMappings->setRowCount(0);

  if (!importedLinks.isEmpty()) {
    QProgressDialog progress("Importing package mappings...", "Cancel", 0,
                             importedLinks.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);

    for (int i = 0; i < importedLinks.size(); ++i) {
      const auto &lp = importedLinks[i];
      addMappingRow(lp.genericName, lp.archPkg, lp.alpinePkg, lp.debianPkg);

      if (i % 5 == 0) {
        progress.setValue(i);
        if (progress.wasCanceled())
          break;
      }
    }
    progress.setValue(importedLinks.size());
  }

  tableMappings->setUpdatesEnabled(true);
  m_isDirty = false;
  m_loading = false;
}

void ProfileEditor::onAddUserClicked() {
    UserDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        UserSpec spec = dialog.spec();
        m_users.append(spec);
        listUsers->addItem(spec.username + (spec.isSudoer ? " (Sudoer)" : ""));
        onFieldChanged();
    }
}

void ProfileEditor::onAddPartitionClicked() {
    PartitionDialog dialog(storageModel, this);
    if (dialog.exec() == QDialog::Accepted) {
        storage::PartitionConfig cfg = dialog.config();
        storageModel->addPartition(cfg);
        refreshPartitionTable();
        onFieldChanged();
    }
}

void ProfileEditor::onEditPartitionClicked() {
    int idx = tablePartitions->currentRow();
    if (idx < 0) return;

    const auto &parts = storageModel->partitions();
    PartitionDialog dialog(storageModel, this);
    dialog.setConfig(parts[idx]);

    if (dialog.exec() == QDialog::Accepted) {
        storageModel->updatePartition(idx, dialog.config());
        refreshPartitionTable();
        onFieldChanged();
    }
}

void ProfileEditor::onRemovePartitionClicked() {
    int idx = tablePartitions->currentRow();
    if (idx < 0) return;

    storageModel->removePartition(idx);
    refreshPartitionTable();
    onFieldChanged();
}

void ProfileEditor::onPartitionDoubleClicked(int row, int) {
    if (row >= 0) onEditPartitionClicked();
}

void ProfileEditor::refreshPartitionTable() {
    tablePartitions->setRowCount(0);
    const auto &parts = storageModel->partitions();

    for (const auto &p : parts) {
        int row = tablePartitions->rowCount();
        tablePartitions->insertRow(row);

        // Determine filesystem color for the number badge
        QString fsColor;
        if (p.fs_type == "vfat") fsColor = "#00F2FE";
        else if (p.fs_type == "ext4") fsColor = "#4FACFE";
        else if (p.fs_type == "btrfs") fsColor = "#00cdac";
        else if (p.fs_type == "swap") fsColor = "#667eea";
        else fsColor = "#888899";

        QTableWidgetItem *numItem = new QTableWidgetItem(QString::number(p.part_num));
        numItem->setTextAlignment(Qt::AlignCenter);
        numItem->setForeground(QColor(fsColor));
        numItem->setFont(QFont("", -1, QFont::Bold));
        tablePartitions->setItem(row, 0, numItem);

        tablePartitions->setItem(row, 1, new QTableWidgetItem(p.size_cmd));
        tablePartitions->setItem(row, 2, new QTableWidgetItem(p.fs_type));
        tablePartitions->setItem(row, 3, new QTableWidgetItem(
            p.mount_point.isEmpty() ? "—" : p.mount_point));
        tablePartitions->setItem(row, 4, new QTableWidgetItem(
            p.label.isEmpty() ? "—" : p.label));
    }
}


void ProfileEditor::onTheoreticalSizeChanged(int size) {
    storageModel->setTotalSizeGb(size);
    onFieldChanged();
}

void ProfileEditor::onAgnosticToggled(bool checked) {
    comboTargetDistro->setEnabled(!checked);
    onFieldChanged();
}

void ProfileEditor::onSearchTextChanged(const QString &text) {
    treeSearchSync->clear();
    if (text.length() < 2) return;

    QString q = text.toLower();
    
    auto scoreMatch = [&](const QString &name) -> int {
        if (name == q) return 0; // Top priority
        if (name.startsWith(q)) return 1;
        return 2;
    };

    auto gatherMatches = [&](const QList<fetchers::PackageInfo> &pkgs) -> QList<fetchers::PackageInfo> {
        QList<fetchers::PackageInfo> matches;
        for (const auto &p : pkgs) {
            if (p.name.contains(q)) matches << p;
        }
        std::sort(matches.begin(), matches.end(), [&](const auto &a, const auto &b){
            int sa = scoreMatch(a.name);
            int sb = scoreMatch(b.name);
            if (sa != sb) return sa < sb;
            return a.name < b.name;
        });
        return matches.mid(0, 50); // Comprehensive limit
    };

    QList<fetchers::PackageInfo> archMatches = gatherMatches(m_syncMgr->archPackages());
    QList<fetchers::PackageInfo> alpineMatches = gatherMatches(m_syncMgr->alpinePackages());
    QList<fetchers::PackageInfo> debianMatches = gatherMatches(m_syncMgr->debianPackages());

    int maxRows = std::max({archMatches.size(), alpineMatches.size(), debianMatches.size()});
    for (int i = 0; i < maxRows; ++i) {
        QTreeWidgetItem *item = new QTreeWidgetItem(treeSearchSync);
        item->setText(0, QString::number(i + 1));
        
        auto formatRow = [&](int index, const QList<fetchers::PackageInfo> &list) {
            if (index < list.size()) {
                const auto &p = list[index];
                item->setText(index == 0 ? 1 : (index == 1 ? 2 : 3), 
                    QString("%1 [%2]").arg(p.name).arg(p.repo));
                item->setToolTip(index == 0 ? 1 : (index == 1 ? 2 : 3), p.description);
            } else {
                item->setText(index == 0 ? 1 : (index == 1 ? 2 : 3), "-");
            }
        };

        // This lambda logic was slightly flawed in the loop, let's just do it directly
        if (i < archMatches.size()) {
            item->setText(1, QString("%1 [%2]").arg(archMatches[i].name).arg(archMatches[i].repo));
            item->setToolTip(1, archMatches[i].description);
        } else item->setText(1, "-");

        if (i < alpineMatches.size()) {
            item->setText(2, QString("%1 [%2]").arg(alpineMatches[i].name).arg(alpineMatches[i].repo));
            item->setToolTip(2, alpineMatches[i].description);
        } else item->setText(2, "-");

        if (i < debianMatches.size()) {
            item->setText(3, QString("%1 [%2]").arg(debianMatches[i].name).arg(debianMatches[i].repo));
            item->setToolTip(3, debianMatches[i].description);
        } else item->setText(3, "-");
    }
}

void ProfileEditor::onDistroChanged(int) {
    if (comboTargetDistro->currentText() == "Alpine Linux") {
        comboInitSystem->setCurrentText("OpenRC");
    } else {
        comboInitSystem->setCurrentText("systemd");
    }
    m_syncMgr->startSync();
    onFieldChanged();
}

void ProfileEditor::onSearchItemDoubleClicked(QTreeWidgetItem *item, int column) {
    if (column < 1 || column > 3) return;
    
    QString raw = item->text(column);
    if (raw == "-") return;

    // Extract clean package name from "name [repo]" format
    auto extractName = [](const QString &cell) -> QString {
        if (cell == "-" || cell.isEmpty()) return "";
        int bracket = cell.indexOf(" [");
        return (bracket != -1) ? cell.left(bracket) : cell;
    };

    QString pkgName = extractName(raw);

    // Gather names from all three distro columns in this row
    QString archName   = extractName(item->text(1));
    QString alpineName = extractName(item->text(2));
    QString debianName = extractName(item->text(3));

    // Collect the non-empty names to check for mismatches
    QStringList presentNames;
    if (!archName.isEmpty())   presentNames << archName;
    if (!alpineName.isEmpty()) presentNames << alpineName;
    if (!debianName.isEmpty()) presentNames << debianName;

    // Detect mismatch: if there are 2+ different names across distros, offer linking
    bool hasMismatch = false;
    if (presentNames.size() >= 2) {
        for (int i = 1; i < presentNames.size(); ++i) {
            if (presentNames[i] != presentNames[0]) {
                hasMismatch = true;
                break;
            }
        }
    }

    // Also detect missing: package exists in one distro but not others
    bool hasMissing = (presentNames.size() < 3 && presentNames.size() >= 1);

    if (hasMismatch || hasMissing) {
        // Build a descriptive message
        QString detail;
        if (hasMismatch) {
            detail = QString("This package has different names across distributions:\n\n"
                             "  Arch:   %1\n  Alpine: %2\n  Debian: %3\n\n"
                             "A simple add may not resolve correctly on all targets.\n"
                             "Would you like to create a Linked Component mapping instead?")
                         .arg(archName.isEmpty()   ? "(not found)" : archName)
                         .arg(alpineName.isEmpty() ? "(not found)" : alpineName)
                         .arg(debianName.isEmpty() ? "(not found)" : debianName);
        } else {
            detail = QString("This package is missing from some distributions:\n\n"
                             "  Arch:   %1\n  Alpine: %2\n  Debian: %3\n\n"
                             "Would you like to create a Linked Component mapping\n"
                             "so you can manually fill the missing slots?")
                         .arg(archName.isEmpty()   ? "(not found)" : archName)
                         .arg(alpineName.isEmpty() ? "(not found)" : alpineName)
                         .arg(debianName.isEmpty() ? "(not found)" : debianName);
        }

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Cross-Distribution Mismatch Detected");
        msgBox.setText(detail);
        msgBox.setIcon(QMessageBox::Question);
        QPushButton *btnLink = msgBox.addButton("Create Linked Component", QMessageBox::AcceptRole);
        QPushButton *btnAdd  = msgBox.addButton("Add As-Is", QMessageBox::RejectRole);
        msgBox.addButton(QMessageBox::Cancel);
        msgBox.setDefaultButton(btnLink);

        // Style the dialog
        msgBox.setStyleSheet(
            "QMessageBox { background-color: #15151E; }"
            "QLabel { color: #E0E0E0; font-size: 13px; }"
            "QPushButton { background-color: #1F1F2E; border: 1px solid #353545; border-radius: 6px; padding: 8px 16px; color: #FFFFFF; }"
            "QPushButton:hover { background-color: #2D2D3D; border-color: #4FACFE; }"
        );

        msgBox.exec();

        if (msgBox.clickedButton() == btnLink) {
            // Transfer to Linked Component Workbench
            addMappingRow(pkgName, archName, alpineName, debianName);
            return;
        } else if (msgBox.clickedButton() == btnAdd) {
            // Fall through to standard add below
        } else {
            return; // Cancelled
        }
    }
    
    // Standard add: just put the package name in the selected list
    bool exists = false;
    for (int i = 0; i < listSelectedPkgs->count(); ++i) {
        QString existing = listSelectedPkgs->item(i)->text().remove(" [Linked]");
        if (existing == pkgName) {
            exists = true;
            break;
        }
    }

    if (!exists) {
        listSelectedPkgs->addItem(pkgName);
        onFieldChanged();
    }
}


void ProfileEditor::onRemovePackageClicked() {
    int row = listSelectedPkgs->currentRow();
    if (row >= 0) {
        delete listSelectedPkgs->takeItem(row);
        onFieldChanged();
    }
}

void ProfileEditor::onSelectedPackageDoubleClicked(QListWidgetItem *item) {
    delete listSelectedPkgs->takeItem(listSelectedPkgs->row(item));
    onFieldChanged();
}

void ProfileEditor::onTreeContextMenu(const QPoint &pos) {
    QTreeWidgetItem *item = treeSearchSync->itemAt(pos);
    if (!item) return;

    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #1A1A24; color: #E0E0E0; border: 1px solid #353545; }"
        "QMenu::item:selected { background-color: #2D2D3D; }"
        "QMenu::separator { background-color: #353545; height: 1px; }"
    );

    QAction *actArch = menu.addAction("Fill Arch Slot");
    QAction *actAlpine = menu.addAction("Fill Alpine Slot");
    QAction *actDebian = menu.addAction("Fill Debian Slot");
    menu.addSeparator();
    QAction *actLinkRow = menu.addAction("Create Linked Component from this row");

    QAction *res = menu.exec(treeSearchSync->viewport()->mapToGlobal(pos));
    if (!res) return;

    // Helper to extract clean name
    auto extractName = [](const QString &cell) -> QString {
        if (cell == "-" || cell.isEmpty()) return "";
        int bracket = cell.indexOf(" [");
        return (bracket != -1) ? cell.left(bracket) : cell;
    };

    if (res == actLinkRow) {
        // Create a linked component from all columns in this row
        QString archName   = extractName(item->text(1));
        QString alpineName = extractName(item->text(2));
        QString debianName = extractName(item->text(3));

        // Use the first non-empty name as the generic name
        QString generic = archName.isEmpty() ? (alpineName.isEmpty() ? debianName : alpineName) : archName;
        if (generic.isEmpty()) return;

        addMappingRow(generic, archName, alpineName, debianName);
        return;
    }

    // Slot filling for a specific column
    int col = (res == actArch) ? 1 : (res == actAlpine ? 2 : 3);
    QString raw = item->text(col);
    if (raw == "-") return;

    QString pkgName = extractName(raw);

    int row = tableMappings->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "No Target", "Please select a row in the Mapping Workbench first.");
        return;
    }

    // Set the combobox value in the target cell
    QComboBox *combo = qobject_cast<QComboBox*>(tableMappings->cellWidget(row, col));
    if (combo) {
        combo->setCurrentText(pkgName);
    }

    // Update m_linkedPkgs
    if (row < m_linkedPkgs.size()) {
        if (col == 1) m_linkedPkgs[row].archPkg = pkgName;
        else if (col == 2) m_linkedPkgs[row].alpinePkg = pkgName;
        else if (col == 3) m_linkedPkgs[row].debianPkg = pkgName;
    }
}

void ProfileEditor::onFillSlot() {}

void ProfileEditor::onAddMappingClicked() {
    bool ok;
    QString generic = QInputDialog::getText(this, "New Linked Component",
        "Enter Generic Package Name (e.g., 'python3'):", QLineEdit::Normal, "", &ok);
    if (ok && !generic.isEmpty()) {
        addMappingRow(generic);
        onFieldChanged();
    }
}

void ProfileEditor::onRemoveMappingClicked() {
    int row = tableMappings->currentRow();
    if (row >= 0 && row < m_linkedPkgs.size()) {
        QString generic = tableMappings->item(row, 0)->text();
        tableMappings->removeRow(row);
        m_linkedPkgs.removeAt(row);

        // Remove from main list
        for (int i = 0; i < listSelectedPkgs->count(); ++i) {
            if (listSelectedPkgs->item(i)->text() == (generic + " [Linked]")) {
                delete listSelectedPkgs->takeItem(i);
                break;
            }
        }
        onFieldChanged();
    }
}

void ProfileEditor::onMappingTableDoubleClicked(int row, int column) {
    if (column == 0) { // Generic name edit (columns 1-3 are comboboxes, they handle editing natively)
        bool ok;
        QString oldName = tableMappings->item(row, 0)->text();
        QString newName = QInputDialog::getText(this, "Edit Generic Name",
            "Enter new generic name:", QLineEdit::Normal, oldName, &ok);
        if (ok && !newName.isEmpty()) {
            tableMappings->item(row, 0)->setText(newName);
            if (row < m_linkedPkgs.size()) {
                m_linkedPkgs[row].genericName = newName;
            }
            
            // Update main list
            for (int i = 0; i < listSelectedPkgs->count(); ++i) {
                if (listSelectedPkgs->item(i)->text() == (oldName + " [Linked]")) {
                    listSelectedPkgs->item(i)->setText(newName + " [Linked]");
                    break;
                }
            }
            onFieldChanged();
        }
    }
}

void ProfileEditor::onImportClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "Import Installer Profile", "", "YAML Files (*.yaml)");
    if (!fileName.isEmpty()) {
        loadProfile(fileName);
        onFieldChanged();
    }
}

void ProfileEditor::onSaveAsClicked() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save Copy of Profile", "uli_profile_copy.yaml", "YAML Files (*.yaml)");
    if (fileName.isEmpty()) return;

    ProfileSpec spec;
    spec.hostname = editHostname->text();
    spec.language = comboLanguage->currentText();
    spec.timezone = comboTimezone->currentText();
    spec.rootPassword = editRootPassword->text();
    spec.bootTarget = comboBootTarget->currentText();
    spec.isEfi = checkIsEfi->isChecked();
    spec.isAgnostic = checkAgnostic->isChecked();
    spec.targetDistro = comboTargetDistro->currentText();
    spec.initSystem = comboInitSystem->currentText();
    spec.networkBackend = comboNetworkBackend->currentText();
    spec.wifiSsid = editWifiSsid->text();
    spec.wifiPassphrase = editWifiPassphrase->text();
    spec.users = m_users;
    spec.partitions = storageModel->partitions();
    spec.bootloaderId = editBootloaderId->text();
    spec.efiDirectory = editEfiDir->text();
    
    QString efiPartText = comboEfiPart->currentText();
    if (efiPartText.contains('#')) {
        spec.efiPart = efiPartText.split('#')[1].split(' ')[0].toInt();
    } else {
        spec.efiPart = 0;
    }

    for (int i=0; i < listSelectedPkgs->count(); i++) {
        spec.additionalPkgs << listSelectedPkgs->item(i)->text().remove(" [Linked]");
    }
    spec.linkedPkgs = m_linkedPkgs;

    if (uli::compositor::ProfileExporter::exportToYaml(fileName, spec)) {
        QMessageBox::information(this, "Copy Saved", "A copy has been saved to: " + fileName);
        // Note: we do NOT call accept() or clearAutosave() here, as this is just a copy action.
    }
}

void ProfileEditor::onFieldChanged() {
    if (m_loading) return;
    m_isDirty = true;
    saveAutosave();
}

void ProfileEditor::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        // Intercept Enter/Return to prevent accidental DIALOG ACCEPT
        event->accept(); 
        return;
    }
    QDialog::keyPressEvent(event);
}

void ProfileEditor::closeEvent(QCloseEvent *event) {
    if (m_isDirty) {
        auto res = QMessageBox::question(this, "Unsaved Changes", 
            "You have unsaved changes in your profile. Are you sure you want to exit and lose them?",
            QMessageBox::Yes | QMessageBox::No);
        if (res == QMessageBox::No) {
            event->ignore();
            return;
        }
    }
    clearAutosave();
    QDialog::closeEvent(event);
}

void ProfileEditor::saveAutosave() {
    ProfileSpec spec;
    spec.hostname = editHostname->text();
    spec.language = comboLanguage->currentText();
    spec.timezone = comboTimezone->currentText();
    spec.rootPassword = editRootPassword->text();
    spec.bootTarget = comboBootTarget->currentText();
    spec.isEfi = checkIsEfi->isChecked();
    spec.isAgnostic = checkAgnostic->isChecked();
    spec.targetDistro = comboTargetDistro->currentText();
    spec.initSystem = comboInitSystem->currentText();
    spec.users = m_users;
    spec.partitions = storageModel->partitions();
    spec.bootloaderId = editBootloaderId->text();
    spec.efiDirectory = editEfiDir->text();
    
    QString efiPartText = comboEfiPart->currentText();
    if (efiPartText.contains('#')) {
        spec.efiPart = efiPartText.split('#')[1].split(' ')[0].toInt();
    } else {
        spec.efiPart = 0;
    }

    for (int i=0; i < listSelectedPkgs->count(); i++) {
        spec.additionalPkgs << listSelectedPkgs->item(i)->text().remove(" [Linked]");
    }
    spec.linkedPkgs = m_linkedPkgs;

    if (uli::compositor::ProfileExporter::exportToYaml(m_currentAutosavePath, spec)) {
        qDebug() << "[Autosave] Session state persisted to:" << m_currentAutosavePath;
    } else {
        qDebug() << "[Autosave] FAILED to write state to:" << m_currentAutosavePath;
    }
}

void ProfileEditor::checkForAutosave() {
    QString localDir = QDir::currentPath();
    QString tmpDir = "/tmp";
    
    QStringList filters;
    filters << ".uli_profile_*.swp";
    
    QStringList found;
    QDir dLocal(localDir);
    for (const auto &f : dLocal.entryList(filters, QDir::Files)) {
        found << localDir + "/" + f;
    }
    
    QDir dTmp(tmpDir);
    for (const auto &f : dTmp.entryList(filters, QDir::Files)) {
        found << tmpDir + "/" + f;
    }
    
    // Exclude our own empty file if it exists
    found.removeAll(m_currentAutosavePath);
    
    if (!found.isEmpty()) {
        handleAutosaveRecovery(found);
    }
}

void ProfileEditor::clearAutosave() {
    QFile::remove(m_currentAutosavePath);
}

QString ProfileEditor::getBestAutosaveDir() const {
    QString exeDir = QCoreApplication::applicationDirPath();
    
    // Per user request: if in /usr/bin or /bin, ALWAYS fallback to /tmp
    if (exeDir.startsWith("/usr/bin") || exeDir.startsWith("/bin")) {
        return "/tmp";
    }
    
    QFileInfo fi(exeDir);
    if (fi.isWritable()) {
        return exeDir;
    }
    
    return "/tmp";
}

QString ProfileEditor::summarizeProfile(const QString &path) const {
    ProfileSpec spec;
    if (ProfileImporter::importFromYaml(path, spec)) {
        QString summary = QString("Hostname: %1\nDistro: %2\nUsers: %3\nPartitions: %4\nPackages: %5")
            .arg(spec.hostname.isEmpty() ? "(none)" : spec.hostname)
            .arg(spec.targetDistro)
            .arg(spec.users.size())
            .arg(spec.partitions.size())
            .arg(spec.additionalPkgs.size() + spec.linkedPkgs.size());
        return summary;
    }
    return "Error parsing session file.";
}

void ProfileEditor::handleAutosaveRecovery(const QStringList &files) {
    QDialog dialog(this);
    dialog.setWindowTitle("Session Recovery Manager");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    layout->addWidget(new QLabel("The compositor detected unsaved progress from previous sessions.\nSelect a point to resume:"));
    
    QListWidget *list = new QListWidget(&dialog);
    QLabel *details = new QLabel("Select a session to see details...", &dialog);
    details->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    details->setMinimumHeight(100);
    details->setStyleSheet("padding: 10px; background: #1A1A24;");
    
    for (const auto &f : files) {
        QFileInfo fi(f);
        QListWidgetItem *item = new QListWidgetItem(fi.fileName(), list);
        item->setData(Qt::UserRole, f);
    }
    
    layout->addWidget(list);
    layout->addWidget(new QLabel("Session Preview:"));
    layout->addWidget(details);
    
    connect(list, &QListWidget::itemSelectionChanged, this, [list, details, this](){
        auto curr = list->currentItem();
        if (curr) {
            details->setText(summarizeProfile(curr->data(Qt::UserRole).toString()));
        }
    });

    QHBoxLayout *btns = new QHBoxLayout();
    QPushButton *btnResume = new QPushButton("Resume Selected Session", &dialog);
    QPushButton *btnDiscard = new QPushButton("Discard All and Start New", &dialog);
    QPushButton *btnIgnore = new QPushButton("Ignore for Now", &dialog);
    
    btns->addWidget(btnResume);
    btns->addWidget(btnDiscard);
    btns->addWidget(btnIgnore);
    layout->addLayout(btns);
    
    btnResume->setStyleSheet("background: #4FACFE; color: black; font-weight: bold;");
    
    connect(btnResume, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(btnDiscard, &QPushButton::clicked, this, [&dialog, files](){
        for (const auto &f : files) QFile::remove(f);
        dialog.reject();
    });
    connect(btnIgnore, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        auto curr = list->currentItem();
        if (curr) {
            loadProfile(curr->data(Qt::UserRole).toString());
        }
    }
}

void ProfileEditor::ensureModelsPopulated(int distroCol) {
  if (!m_syncMgr) {
    qWarning() << "ProfileEditor: syncMgr is null, cannot populate models";
    return;
  }

  QStringListModel *targetModel = (distroCol == 1)   ? m_modelArch
                                  : (distroCol == 2) ? m_modelAlpine
                                                     : m_modelDebian;

  if (!targetModel) {
    qWarning() << "ProfileEditor: targetModel is null for distroCol" << distroCol;
    return;
  }

  if (targetModel->stringList().isEmpty()) {
    QStringList *targetCache = (distroCol == 1)   ? &m_cachedArchPkgs
                               : (distroCol == 2) ? &m_cachedAlpinePkgs
                                                  : &m_cachedDebianPkgs;

    if (targetCache->isEmpty()) {
      // PERFORMANCE: Fetch and copy to local cache once
      // Fixed: Removed unsafe ternary reference binding to prevent dangling pointers
      QList<fetchers::PackageInfo> pkgs;
      if (distroCol == 1)      pkgs = m_syncMgr->archPackages();
      else if (distroCol == 2) pkgs = m_syncMgr->alpinePackages();
      else                     pkgs = m_syncMgr->debianPackages();

      targetCache->reserve(pkgs.size());
      for (const auto &p : pkgs) {
        *targetCache << p.name;
      }
      targetCache->sort();
      targetCache->removeDuplicates();
    }
    targetModel->setStringList(*targetCache);
  }
}

} // namespace ui
} // namespace compositor
} // namespace uli
