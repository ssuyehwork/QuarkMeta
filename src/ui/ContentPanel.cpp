#include "ContentPanel.h"
#include "DropTreeView.h" 
#include "DropListView.h" 
#include "DropJustifiedView.h"
#include "ThumbnailDelegate.h"
#include "TreeItemDelegate.h"
#include "UiHelper.h"
#include "BatchRenameDialog.h"
#include "../util/ThumbnailPipelineService.h"
#include "../meta/DuplicateDetectorService.h"
#include "../meta/MetaCacheDecorator.h"
#include "../core/DiskScanService.h"
#include "../core/AppConfig.h"
#include "../core/CoreController.h"
#include <QHeaderView> 
#include <QScrollBar> 
#include <QDesktopServices> 
#include <QUrl> 
#include <QtConcurrent> 
#include <QCoreApplication>
 
namespace QuarkMeta { 
 
ContentPanel::ContentPanel(QWidget* parent) : QFrame(parent) {
    setObjectName("EditorContainer"); 
    setAttribute(Qt::WA_StyledBackground, true); 
    setMinimumWidth(230); 
 
    m_mainLayout = new QVBoxLayout(this); 
    m_mainLayout->setContentsMargins(0, 0, 0, 0); 
    m_mainLayout->setSpacing(0); 
 
    m_contextMenuController = new ContentContextMenuController(this, this);
    m_actionController = new ContentActionController(this);

    m_diskModel = new DiskItemModel(this);
    m_model = m_diskModel;

    m_folderProxyModel = new FolderProxyModel(this);
    m_folderProxyModel->setSourceModel(m_model);

    m_fileProxyModel = new FileProxyModel(this);
    m_fileProxyModel->setSourceModel(m_model);
 
    m_visibleTimer = new QTimer(this);
    m_visibleTimer->setSingleShot(true);
    m_visibleTimer->setInterval(60); 
 
    m_zoomLevel = AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt(); 
    m_showFolders = AppConfig::instance().getValue("ContentPanel/ShowFolders", true).toBool();
    m_showFiles = AppConfig::instance().getValue("ContentPanel/ShowFiles", true).toBool();
    m_showHidden = AppConfig::instance().getValue("ContentPanel/ShowHidden", false).toBool();

    m_currentFilter.showFolders = m_showFolders;
    m_currentFilter.showFiles = m_showFiles;
    m_currentFilter.showHidden = m_showHidden;

    m_sortType = static_cast<SortType>(AppConfig::instance().getValue("ContentPanel/RightClickSortType", SortByName).toInt());
    m_sortOrder = static_cast<Qt::SortOrder>(AppConfig::instance().getValue("ContentPanel/RightClickSortOrder", Qt::AscendingOrder).toInt());
 
    initUi(); 
    initDualContainers();
 
    int savedMode = AppConfig::instance().getValue("ContentPanel/ViewMode", static_cast<int>(GridView)).toInt();
    setViewMode(static_cast<ViewMode>(savedMode));
} 
 
void ContentPanel::initUi() { 
    QWidget* titleBar = new QWidget(this); 
    titleBar->setObjectName("ContainerHeader"); 
    titleBar->setFixedHeight(32); 
    titleBar->setStyleSheet("QWidget#ContainerHeader { background-color: #252526; border-bottom: 1px solid #333333; }");
    QHBoxLayout* titleL = new QHBoxLayout(titleBar); 
    titleL->setContentsMargins(15, 0, 5, 0);
    titleL->setSpacing(5);
 
    QLabel* iconLabel = new QLabel(titleBar); 
    iconLabel->setPixmap(UiHelper::getIcon("eye", QColor("#41F2F2"), 18).pixmap(18, 18)); 
    titleL->addWidget(iconLabel); 
 
    QLabel* titleLabel = new QLabel("内容", titleBar); 
    titleLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #41F2F2; background: transparent; border: none;"); 
    titleL->addWidget(titleLabel); 
    titleL->addStretch(); 
 
    m_btnToggleHidden = new QPushButton(titleBar);
    m_btnToggleHidden->setCheckable(true);
    m_btnToggleHidden->setFixedSize(24, 24);
    m_btnToggleHidden->setChecked(m_showHidden);
    m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", m_showHidden ? QColor("#3498db") : QColor("#888888"), 16));
    m_btnToggleHidden->setProperty("tooltipText", "显示/隐藏属性为隐藏的项目");
    m_btnToggleHidden->setStyleSheet("QPushButton { background: transparent; border: 1px solid #444; border-radius: 4px; } QPushButton:checked { background: #3E3E42; border-color: #3498db; }");
    connect(m_btnToggleHidden, &QPushButton::clicked, [this]() {
        m_showHidden = m_btnToggleHidden->isChecked();
        m_btnToggleHidden->setIcon(UiHelper::getIcon("eye", m_showHidden ? QColor("#3498db") : QColor("#888888"), 16));
        AppConfig::instance().setValue("ContentPanel/ShowHidden", m_showHidden);
        applyFilters();
    });
    titleL->addWidget(m_btnToggleHidden);
 
    m_btnToggleFolders = new QPushButton(titleBar);
    m_btnToggleFolders->setCheckable(true);
    m_btnToggleFolders->setFixedSize(24, 24);
    m_btnToggleFolders->setChecked(m_showFolders);
    m_btnToggleFolders->setIcon(UiHelper::getIcon("folder_filled", m_showFolders ? QColor("#FDB70A") : QColor("#B0B0B0"), 16));
    m_btnToggleFolders->setProperty("tooltipText", "显示/隐藏文件夹");
    m_btnToggleFolders->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 4px; } QPushButton:checked { background: #3E3E42; }");
    connect(m_btnToggleFolders, &QPushButton::clicked, [this]() {
        m_showFolders = m_btnToggleFolders->isChecked();
        m_btnToggleFolders->setIcon(UiHelper::getIcon("folder_filled", m_showFolders ? QColor("#FDB70A") : QColor("#B0B0B0"), 16));
        AppConfig::instance().setValue("ContentPanel/ShowFolders", m_showFolders);
        applyFilters();
    });
    titleL->addWidget(m_btnToggleFolders);
 
    m_btnToggleFiles = new QPushButton(titleBar);
    m_btnToggleFiles->setCheckable(true);
    m_btnToggleFiles->setFixedSize(24, 24);
    m_btnToggleFiles->setChecked(m_showFiles);
    m_btnToggleFiles->setIcon(UiHelper::getIcon("file", m_showFiles ? QColor("#2ecc71") : QColor("#B0B0B0"), 16));
    m_btnToggleFiles->setProperty("tooltipText", "显示/隐藏文件");
    m_btnToggleFiles->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 4px; } QPushButton:checked { background: #3E3E42; }");
    connect(m_btnToggleFiles, &QPushButton::clicked, [this]() {
        m_showFiles = m_btnToggleFiles->isChecked();
        m_btnToggleFiles->setIcon(UiHelper::getIcon("file", m_showFiles ? QColor("#2ecc71") : QColor("#B0B0B0"), 16));
        AppConfig::instance().setValue("ContentPanel/ShowFiles", m_showFiles);
        applyFilters();
    });
    titleL->addWidget(m_btnToggleFiles);
 
    m_btnLayers = new QPushButton(titleBar);
    m_btnLayers->setCheckable(true);
    m_btnLayers->setFixedSize(24, 24);
    m_btnLayers->setIcon(UiHelper::getIcon("layers", QColor("#2ecc71"), 18));
    m_btnLayers->setProperty("tooltipText", "显示子文件夹中的项目");
    m_btnLayers->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 4px; } QPushButton:checked { background: #3E3E42; }");
    titleL->addWidget(m_btnLayers);
 
    m_mainLayout->addWidget(titleBar);
 
    QWidget* bodyWidget = new QWidget(this);
    m_centerLayout = new QVBoxLayout(bodyWidget);
    m_centerLayout->setContentsMargins(4, 4, 0, 4);
    m_centerLayout->setSpacing(8);

    m_mainLayout->addWidget(bodyWidget, 1);
} 
 
void ContentPanel::initDualContainers() {
    // 1. 上方文件夹容器
    m_folderContainer = new QWidget(this);
    QVBoxLayout* folderL = new QVBoxLayout(m_folderContainer);
    folderL->setContentsMargins(0, 0, 0, 0);
 
    m_folderViewStack = new QStackedWidget(m_folderContainer);
    m_folderGridView = new DropJustifiedView(this);
    m_folderGridView->setModel(m_folderProxyModel);
    m_folderGridView->setItemDelegate(new ThumbnailDelegate(this));
    m_folderGridView->setStyleSheet("background: transparent; border: none; outline: none;");
 
    DropTreeView* folderTree = new DropTreeView(this);
    folderTree->setModel(m_folderProxyModel);
    folderTree->setItemDelegate(new TreeItemDelegate(this, false, true));
    folderTree->setHeaderHidden(true);
    folderTree->setStyleSheet("QTreeView { background: transparent; border: none; outline: none; }");
    m_folderListView = folderTree;
 
    m_folderViewStack->addWidget(m_folderGridView);
    m_folderViewStack->addWidget(m_folderListView);
    folderL->addWidget(m_folderViewStack);
 
    // 2. 下方文件容器
    m_fileContainer = new QWidget(this);
    QVBoxLayout* fileL = new QVBoxLayout(m_fileContainer);
    fileL->setContentsMargins(0, 0, 0, 0);
 
    m_fileViewStack = new QStackedWidget(m_fileContainer);
    m_fileGridView = new DropJustifiedView(this);
    m_fileGridView->setModel(m_fileProxyModel);
    m_fileGridView->setItemDelegate(new ThumbnailDelegate(this));
    m_fileGridView->setStyleSheet("background: transparent; border: none; outline: none;");
 
    m_fileListView = new DropTreeView(this);
    m_fileListView->setModel(m_fileProxyModel);
    m_fileListView->setItemDelegate(new TreeItemDelegate(this, true, true));
    m_fileListView->setStyleSheet("QTreeView { background: transparent; border: none; outline: none; }");
 
    m_fileViewStack->addWidget(m_fileGridView);
    m_fileViewStack->addWidget(m_fileListView);
    fileL->addWidget(m_fileViewStack);
 
    m_centerLayout->addWidget(m_folderContainer, 0);
    m_centerLayout->addWidget(m_fileContainer, 1);
 
    // 绑定右键菜单与双击
    auto bindEvents = [this](QAbstractItemView* view) {
        view->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(view, &QWidget::customContextMenuRequested, this, [this, view](const QPoint& pos) {
            m_contextMenuController->showContextMenu(view, pos, m_currentPath, m_currentCategoryType);
        });
        connect(view, &QAbstractItemView::doubleClicked, this, &ContentPanel::onDoubleClicked);
    };
 
    bindEvents(m_folderGridView);
    bindEvents(m_folderListView);
    bindEvents(m_fileGridView);
    bindEvents(m_fileListView);
 
    // 双向互斥选区
    auto onSelect = [this]() { emitSelectionChangedSignal(); };
    connect(m_folderGridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, onSelect);
    connect(m_folderListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, onSelect);
    connect(m_fileGridView->selectionModel(), &QItemSelectionModel::selectionChanged, this, onSelect);
    connect(m_fileListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, onSelect);
}
 
void ContentPanel::updateDualContainersVisibility() {
    bool hasFolders = (m_folderProxyModel && m_folderProxyModel->rowCount() > 0);
    m_folderContainer->setVisible(hasFolders && m_showFolders);
 
    bool hasFiles = (m_fileProxyModel && m_fileProxyModel->rowCount() > 0);
    m_fileContainer->setVisible(hasFiles && m_showFiles);
}
 
QModelIndexList ContentPanel::getSelectedIndexes() const {
    QModelIndexList result;
    if (m_folderContainer && m_folderContainer->isVisible()) {
        auto* sel = (m_currentViewMode == ListView) ? m_folderListView->selectionModel() : m_folderGridView->selectionModel();
        if (sel) result.append(sel->selectedIndexes());
    } 
    if (m_fileContainer && m_fileContainer->isVisible()) {
        auto* sel = (m_currentViewMode == ListView) ? m_fileListView->selectionModel() : m_fileGridView->selectionModel();
        if (sel) result.append(sel->selectedIndexes());
    } 
    return result;
}
 
QStringList ContentPanel::getSelectedPaths() const {
    QStringList paths;
    for (const auto& idx : getSelectedIndexes()) {
        if (idx.column() == 0) {
            QString p = idx.data(PathRole).toString();
            if (!p.isEmpty() && !paths.contains(p)) paths << p;
        } 
    } 
    return paths;
} 
 
QList<int> ContentPanel::getSelectedTrashIds() const {
    QList<int> ids;
    for (const auto& idx : getSelectedIndexes()) {
        if (idx.column() == 0 && idx.data(IsDiskTrashRole).toBool()) {
            ids << idx.data(DiskTrashIdRole).toInt();
        } 
    } 
    return ids;
}
 
void ContentPanel::setViewMode(ViewMode mode) {
    m_currentViewMode = mode;
    int page = (mode == ListView) ? 1 : 0;
    m_folderViewStack->setCurrentIndex(page);
    m_fileViewStack->setCurrentIndex(page);
 
    AppConfig::instance().setValue("ContentPanel/ViewMode", static_cast<int>(mode));
    emit viewModeChanged(mode);
} 
 
void ContentPanel::setSortType(SortType type) {
    m_sortType = type;
    AppConfig::instance().setValue("ContentPanel/RightClickSortType", static_cast<int>(type));
    m_folderProxyModel->invalidate();
    m_folderProxyModel->sort(0, m_sortOrder);
    m_fileProxyModel->invalidate();
    m_fileProxyModel->sort(0, m_sortOrder);
} 
 
void ContentPanel::setSortOrder(Qt::SortOrder order) {
    m_sortOrder = order;
    AppConfig::instance().setValue("ContentPanel/RightClickSortOrder", static_cast<int>(order));
    m_folderProxyModel->sort(0, order);
    m_fileProxyModel->sort(0, order);
} 
 
void ContentPanel::applyFilters(const FilterState& state) {
    m_currentFilter = state;
    m_currentFilter.showFolders = m_showFolders;
    m_currentFilter.showFiles = m_showFiles;
    m_currentFilter.showHidden = m_showHidden;
 
    m_folderProxyModel->currentFilter = m_currentFilter;
    m_folderProxyModel->updateFilter();
 
    m_fileProxyModel->currentFilter = m_currentFilter;
    m_fileProxyModel->updateFilter();
 
    updateDualContainersVisibility();
    updateStatusBarStats();
} 
 
void ContentPanel::applyFilters() {
    applyFilters(m_currentFilter);
} 
 
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    m_currentPath = path;
    int reqId = ++m_loadRequestId;
 
    ThumbnailPipelineService::instance().cancelAll();
    m_diskModel->incrementGeneration();
 
    QPointer<ContentPanel> weakThis(this);
    (void)QtConcurrent::run([weakThis, path, recursive, reqId]() {
        if (!weakThis) return;
        DiskScanService::scanDirectoryChunked(
            path, recursive,
            [weakThis, reqId](std::vector<ItemRecord>&& chunk, bool isFirstChunk) {
                if (!weakThis || weakThis->m_loadRequestId != reqId) return;
                QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis, chunkData = std::move(chunk), isFirstChunk]() mutable {
                    if (!weakThis) return;
                    if (isFirstChunk) {
                        weakThis->m_model->setRecords(std::move(chunkData));
                        weakThis->m_folderProxyModel->sort(0, weakThis->m_sortOrder);
                        weakThis->m_fileProxyModel->sort(0, weakThis->m_sortOrder);
                    } else {
                        weakThis->m_model->appendRecords(std::move(chunkData));
                    } 
                    weakThis->updateDualContainersVisibility();
                    weakThis->recalculateAndEmitStats();
                }, Qt::QueuedConnection);
            },
            [weakThis, reqId]() { return weakThis && (weakThis->m_loadRequestId == reqId); }
        );
    });
} 
 
void ContentPanel::createNewItem(const QString& type) {
    QString newPath;
    if (m_actionController->createNewItem(m_currentPath, type, newPath)) {
        setPendingSelectName(newPath, true);
        refreshAll();
    } 
} 
 
void ContentPanel::performBatchRename() { 
    auto paths = getSelectedPaths();
    if (paths.isEmpty()) return;
 
    std::vector<std::wstring> stdPaths;
    for (const QString& p : paths) stdPaths.push_back(QDir::toNativeSeparators(p).toStdWString());
 
    BatchRenameDialog dlg(stdPaths, this);
    if (dlg.exec() == QDialog::Accepted) { 
        refreshAll(); 
    }
} 
 
void ContentPanel::onDoubleClicked(const QModelIndex& index) { 
    if (!index.isValid()) return; 
    QString path = index.data(PathRole).toString(); 
    if (path.isEmpty()) return; 
 
    if (QFileInfo(path).isDir()) {
        emit directorySelected(path);
    } else { 
        emit requestQuickLook(path);
    } 
} 
 
void ContentPanel::emitSelectionChangedSignal() {
    emit selectionChanged(getSelectedPaths());
    updateStatusBarStats();
} 
 
void ContentPanel::refreshAll() {
    loadDirectory(m_currentPath, m_isRecursive);
}
 
void ContentPanel::updateItemMetadata(const QString& path) {
    if (m_model) m_model->updateRecordMetadata(path);
}
 
void ContentPanel::migrateModelCache(const QString& oldPath, const QString& newPath) {
    if (m_model) m_model->migrateCache(oldPath, newPath);
}
 
void ContentPanel::clearFolderCache(const QString& folderPath) {
    if (m_model) m_model->clearCacheForFolder(folderPath);
} 
 
void ContentPanel::search(const QString& query) {
    m_currentFilter.keyword = query;
    applyFilters(); 
} 
 
void ContentPanel::loadCategory(const QString& categoryType) {
    m_currentCategoryType = categoryType;
    if (categoryType == "trash") m_currentPath = "trash://";
} 
 
void ContentPanel::recalculateAndEmitStats() {
    // 委托后台统计
} 
 
void ContentPanel::updateStatusBarStats() {
    int total = (m_folderProxyModel ? m_folderProxyModel->rowCount() : 0) + (m_fileProxyModel ? m_fileProxyModel->rowCount() : 0);
    emit statusBarStatsUpdated(0, 0, total);
} 
 
void ContentPanel::refreshVisibleThumbnails() {
    // 委托 ThumbnailPipelineService
} 
 
void ContentPanel::updateGridSize() {}
void ContentPanel::setZoomLevel(int level) { m_zoomLevel = level; emit zoomLevelChanged(level); }
void ContentPanel::setPendingSelectName(const QString& name, bool edit) { m_pendingSelectNames.insert(name); m_isPendingEdit = edit; }
QString ContentPanel::getAdjacentFilePath(const QString&, int) { return QString(); }
void ContentPanel::selectAndScrollToPath(const QString&) {}
bool ContentPanel::eventFilter(QObject* obj, QEvent* event) { return QFrame::eventFilter(obj, event); }
void ContentPanel::wheelEvent(QWheelEvent* event) { QFrame::wheelEvent(event); }

} // namespace QuarkMeta
