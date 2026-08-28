#pragma once

#include <QFrame>
#include <QPointer>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include "models/DiskItemModel.h"
#include "models/FolderProxyModel.h"
#include "models/FileProxyModel.h"
#include "controllers/ContentContextMenuController.h"
#include "controllers/ContentActionController.h"
#include "ScanStats.h"

namespace QuarkMeta {

class ContentPanel : public QFrame {
    Q_OBJECT

public:
    enum ViewMode { ListView, GridView, JustifiedViewMode };
    enum SortType { SortByName, SortByCreateDate, SortByModifyDate, SortByExtension, SortBySize, SortByDimension, SortByRating, SortByAddedDate };
    enum ContextAction {
        ActionOpen, ActionOpenDefault, ActionShowInExplorer, ActionNewFolder, ActionNewMd, ActionNewTxt,
        ActionPin, ActionUnpin, ActionColorTag, ActionEncrypt, ActionDecrypt, ActionChangePwd,
        ActionBatchRename, ActionRename, ActionCopy, ActionCut, ActionPaste, ActionDelete,
        ActionPermanentDelete, ActionSecureDelete, ActionRestore, ActionRestoreAll, ActionEmptyTrash,
        ActionCopyName, ActionCopyPath, ActionAddToFavorites, ActionRefresh, ActionReextractThumbnail, ActionBatchCreate
    };

    explicit ContentPanel(QWidget* parent = nullptr);
    ~ContentPanel() override = default;

    void deferredInit() {}

    SortType currentSortType() const { return m_sortType; }
    Qt::SortOrder currentSortOrder() const { return m_sortOrder; }
    ViewMode currentViewMode() const { return m_currentViewMode; }
    QString getCurrentCategoryType() const { return m_currentCategoryType; }
    QString currentPath() const { return m_currentPath; }

    void setSortType(SortType type);
    void setSortOrder(Qt::SortOrder order);

    QModelIndexList getSelectedIndexes() const;
    QStringList getSelectedPaths() const;
    QList<int> getSelectedTrashIds() const;
    QString getAdjacentFilePath(const QString& currentPath, int delta);

    void selectAndScrollToPath(const QString& path);
    void setPendingSelectName(const QString& name, bool edit = false);

    ItemModelBase* model() const { return m_model; }
    QSortFilterProxyModel* getProxyModel() const { return m_fileProxyModel; }

    void performBatchRename();

signals:
    void zoomLevelChanged(int level);
    void viewModeChanged(ViewMode mode);
    void requestQuickLook(const QString& path);
    void selectionChanged(const QStringList& paths);
    void directorySelected(const QString& path);
    void requestAddFavorite(const QStringList& paths);
    void dataSourceChanged(const QString& source);
    void directoryStatsReady(const QuarkMeta::ScanStats& stats);
    void statusBarStatsUpdated(int fileCount, int folderCount, int totalCount);

public slots:
    void setViewMode(ViewMode mode);
    void setZoomLevel(int level);
    void loadDirectory(const QString& path, bool recursive = false);
    void loadCategory(const QString& categoryType);
    void refreshAll();
    void updateItemMetadata(const QString& path);
    void migrateModelCache(const QString& oldPath, const QString& newPath);
    void clearFolderCache(const QString& folderPath);
    void search(const QString& query);
    void applyFilters(const FilterState& state);
    void applyFilters();
    void createNewItem(const QString& type);
    void onDoubleClicked(const QModelIndex& index);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void initUi();
    void initDualContainers();
    void updateDualContainersVisibility();
    void updateGridSize();
    void recalculateAndEmitStats();
    void updateStatusBarStats();
    void emitSelectionChangedSignal();
    void refreshVisibleThumbnails();

    QVBoxLayout* m_mainLayout = nullptr;
    QVBoxLayout* m_centerLayout = nullptr;

    QWidget* m_folderContainer = nullptr;
    QStackedWidget* m_folderViewStack = nullptr;
    QAbstractItemView* m_folderGridView = nullptr;
    QAbstractItemView* m_folderListView = nullptr;

    QWidget* m_fileContainer = nullptr;
    QStackedWidget* m_fileViewStack = nullptr;
    QAbstractItemView* m_fileGridView = nullptr;
    QAbstractItemView* m_fileListView = nullptr;

    DiskItemModel* m_diskModel = nullptr;
    ItemModelBase* m_model = nullptr;
    FolderProxyModel* m_folderProxyModel = nullptr;
    FileProxyModel* m_fileProxyModel = nullptr;

    ContentContextMenuController* m_contextMenuController = nullptr;
    ContentActionController* m_actionController = nullptr;

    FilterState m_currentFilter;
    int m_zoomLevel = 96;
    QString m_currentPath;
    QString m_currentCategoryType;
    bool m_isRecursive = false;
    bool m_showFolders = true;
    bool m_showFiles = true;
    bool m_showHidden = false;
    ViewMode m_currentViewMode = GridView;
    SortType m_sortType = SortByName;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;

    QTimer* m_selectionTimer = nullptr;
    QTimer* m_visibleTimer = nullptr;
    std::atomic<int> m_loadRequestId{0};
    QSet<QString> m_pendingSelectNames;
    bool m_isPendingEdit = false;

    QPushButton* m_btnToggleHidden = nullptr;
    QPushButton* m_btnToggleFolders = nullptr;
    QPushButton* m_btnToggleFiles = nullptr;
    QPushButton* m_btnLayers = nullptr;
};

} // namespace QuarkMeta
