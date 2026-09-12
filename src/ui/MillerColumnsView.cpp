#include "MillerColumnsView.h"
#include "models/DiskItemModel.h"
#include "models/FilterProxyModel.h"
#include "../core/ModelContract.h"
#include "../core/DiskScanService.h"
#include "../meta/MetaCacheDecorator.h"
#include "UiHelper.h"

#include <QScrollBar>
#include <QFileInfo>
#include <QDir>
#include <QTimer>

namespace QuarkMeta {

// --- MillerColumnPane Implementation ---

MillerColumnPane::MillerColumnPane(const QString& path, QWidget* parent)
    : QWidget(parent), m_path(path) {
    initPane();
}

void MillerColumnPane::initPane() {
    setFixedWidth(220);
    setStyleSheet("QWidget { background-color: #1E1E1E; border-right: 1px solid #333333; }");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_listView = new QListView(this);
    m_listView->setObjectName("ColumnViewPaneListView");
    m_listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_listView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_listView->setStyleSheet(
        "QListView#ColumnViewPaneListView { background-color: #1E1E1E; border: none; outline: none; }"
        "QListView#ColumnViewPaneListView::item { height: 26px; border: none; color: #CCCCCC; }"
        "QListView#ColumnViewPaneListView::item:hover { background-color: #2A2D2E; }"
        "QListView#ColumnViewPaneListView::item:selected { background-color: #04395E; color: #FFFFFF; }"
    );

    m_diskModel = new DiskItemModel(this);
    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_diskModel);
    m_listView->setModel(m_proxyModel);

    std::function<bool()> cancelCheck = nullptr;
    std::vector<ItemRecord> records = DiskScanService::scanDirectory(m_path, false, cancelCheck);
    MetaCacheDecorator::decorate(records);
    m_diskModel->setRecords(records);

    connect(m_listView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        QModelIndexList selected = m_listView->selectionModel()->selectedIndexes();
        if (selected.isEmpty()) return;

        QModelIndex proxyIdx = selected.first();
        QModelIndex srcIdx = m_proxyModel->mapToSource(proxyIdx);
        QString itemPath = srcIdx.data(QuarkMeta::PathRole).toString();
        bool isDir = srcIdx.data(QuarkMeta::TypeRole).toString() == "folder" || QFileInfo(itemPath).isDir();

        if (isDir) {
            emit folderSelected(itemPath);
        } else {
            emit fileSelected(itemPath);
        }
    });

    connect(m_listView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        QModelIndex srcIdx = m_proxyModel->mapToSource(index);
        QString itemPath = srcIdx.data(QuarkMeta::PathRole).toString();
        if (!QFileInfo(itemPath).isDir()) {
            emit fileDoubleClicked(itemPath);
        }
    });

    layout->addWidget(m_listView);
}


// --- MillerColumnsView Implementation ---

MillerColumnsView::MillerColumnsView(QWidget* parent)
    : QAbstractItemView(parent) {
    setObjectName("MillerColumnsView");
    setStyleSheet("QWidget#MillerColumnsView { background-color: #1E1E1E; border: none; }");

    QVBoxLayout* selfLayout = new QVBoxLayout(this);
    selfLayout->setContentsMargins(0, 0, 0, 0);
    selfLayout->setSpacing(0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setObjectName("ColumnViewScrollArea");
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet("QScrollArea#ColumnViewScrollArea { background-color: #1E1E1E; border: none; }");

    m_containerWidget = new QWidget(m_scrollArea);
    m_containerWidget->setStyleSheet("QWidget { background-color: #1E1E1E; }");

    m_containerLayout = new QHBoxLayout(m_containerWidget);
    m_containerLayout->setContentsMargins(0, 0, 0, 0);
    m_containerLayout->setSpacing(0);
    m_containerLayout->addStretch();

    m_scrollArea->setWidget(m_containerWidget);
    selfLayout->addWidget(m_scrollArea);
}

void MillerColumnsView::setRootPath(const QString& rootPath) {
    m_rootPath = rootPath;

    // 清空现有所有列
    truncateColumnsAfter(-1);

    if (!m_rootPath.isEmpty()) {
        appendColumn(m_rootPath, -1);
    }
}

void MillerColumnsView::appendColumn(const QString& folderPath, int parentPaneIndex) {
    truncateColumnsAfter(parentPaneIndex);

    MillerColumnPane* pane = new MillerColumnPane(folderPath, m_containerWidget);
    int paneIndex = m_panes.size();
    m_panes.append(pane);

    // 插在 addStretch 之前
    m_containerLayout->insertWidget(m_containerLayout->count() - 1, pane);

    connect(pane, &MillerColumnPane::folderSelected, this, [this, paneIndex](const QString& subFolderPath) {
        appendColumn(subFolderPath, paneIndex);
        emit directoryNavigated(subFolderPath);
    });

    connect(pane, &MillerColumnPane::fileSelected, this, [this, paneIndex](const QString& filePath) {
        truncateColumnsAfter(paneIndex);
        emit fileSelected(filePath);
    });

    connect(pane, &MillerColumnPane::fileDoubleClicked, this, [this](const QString& filePath) {
        emit fileActivated(filePath);
        emit doubleClicked(QModelIndex());
    });

    scrollToRightmostPane();
}

void MillerColumnsView::truncateColumnsAfter(int paneIndex) {
    while (m_panes.size() > paneIndex + 1) {
        MillerColumnPane* pane = m_panes.takeLast();
        m_containerLayout->removeWidget(pane);
        pane->deleteLater();
    }
}

void MillerColumnsView::scrollToRightmostPane() {
    QTimer::singleShot(30, this, [this]() {
        if (m_scrollArea && m_scrollArea->horizontalScrollBar()) {
            m_scrollArea->horizontalScrollBar()->setValue(m_scrollArea->horizontalScrollBar()->maximum());
        }
    });
}

// QAbstractItemView Required Dummy Implementations
QRect MillerColumnsView::visualRect(const QModelIndex&) const { return QRect(); }
void MillerColumnsView::scrollTo(const QModelIndex&, ScrollHint) {}
QModelIndex MillerColumnsView::indexAt(const QPoint&) const { return QModelIndex(); }
QModelIndex MillerColumnsView::moveCursor(CursorAction, Qt::KeyboardModifiers) { return QModelIndex(); }
int MillerColumnsView::horizontalOffset() const { return 0; }
int MillerColumnsView::verticalOffset() const { return 0; }
bool MillerColumnsView::isIndexHidden(const QModelIndex&) const { return false; }
void MillerColumnsView::setSelection(const QRect&, QItemSelectionModel::SelectionFlags) {}
QRegion MillerColumnsView::visualRegionForSelection(const QItemSelection&) const { return QRegion(); }

} // namespace QuarkMeta
