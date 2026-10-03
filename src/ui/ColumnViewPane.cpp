#include "ColumnViewPane.h"
#include "ContentPanel.h"
#include "ColumnViewWidget.h"
#include "ColumnItemDelegate.h"
#include "../core/DiskScanService.h"
#include "../meta/MetaCacheDecorator.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>

namespace QuarkMeta {

ColumnViewPane::ColumnViewPane(const QString& path, ContentPanel* contentPanel, QWidget* parent)
    : QWidget(parent), m_path(path), m_contentPanel(contentPanel) 
{
    setObjectName("ColumnViewPane");
    setAttribute(Qt::WA_StyledBackground, true);
    setMinimumWidth(220);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 1, 1, 0);
    layout->setSpacing(0);

    m_paneScrollArea = new QScrollArea(this);
    m_paneScrollArea->setObjectName("ColumnPaneScrollArea");
    m_paneScrollArea->setWidgetResizable(true);
    m_paneScrollArea->setFrameShape(QFrame::NoFrame);
    m_paneScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_paneScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_paneScrollArea->setContextMenuPolicy(Qt::CustomContextMenu);

    m_model = new DiskItemModel(this);
    m_model->setCurrentPath(path);

    m_proxyModel = new FilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterKeyColumn(0);
    m_proxyModel->setDynamicSortFilter(true);

    m_containerWidget = new QWidget(this);
    m_containerLayout = new QVBoxLayout(m_containerWidget);
    m_containerLayout->setContentsMargins(0, 0, 0, 0);
    m_containerLayout->setSpacing(0);

    m_folderHeader = new FolderSectionHeaderBar(m_containerWidget);
    m_folderHeader->hide();
    m_containerLayout->addWidget(m_folderHeader);

    m_unifiedListView = new DropListView();
    m_unifiedListView->setObjectName("ColumnViewPaneListView");
    m_unifiedListView->setFrameShape(QFrame::NoFrame);
    m_unifiedListView->setFocusPolicy(Qt::StrongFocus);
    m_unifiedListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_unifiedListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_unifiedListView->setDragEnabled(true);
    m_unifiedListView->setAcceptDrops(true);
    m_unifiedListView->setDropIndicatorShown(true);
    m_unifiedListView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_unifiedListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_unifiedListView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_unifiedListView->setModel(m_proxyModel);
    m_unifiedListView->setItemDelegate(new ColumnItemDelegate(this));
    m_containerLayout->addWidget(m_unifiedListView);

    m_fileHeader = new FileSectionHeaderBar(m_containerWidget);
    m_fileHeader->hide();
    m_containerLayout->addWidget(m_fileHeader);

    m_paneScrollArea->setWidget(m_containerWidget);
    layout->addWidget(m_paneScrollArea);

    auto handlePaneBlankContextMenu = [this](const QPoint& pos, QWidget* sourceWidget) {
        if (!m_contentPanel) return;
        QPoint globalPos = sourceWidget ? sourceWidget->mapToGlobal(pos) : QCursor::pos();
        DropListView* targetView = m_unifiedListView;
        if (targetView && targetView->viewport()) {
            QPoint viewPos = targetView->viewport()->mapFromGlobal(globalPos);
            m_contentPanel->onCustomContextMenuRequested(targetView, viewPos);
        }
    };

    connect(m_paneScrollArea, &QWidget::customContextMenuRequested, this, [this, handlePaneBlankContextMenu](const QPoint& pos) {
        handlePaneBlankContextMenu(pos, m_paneScrollArea);
    });
    connect(m_containerWidget, &QWidget::customContextMenuRequested, this, [this, handlePaneBlankContextMenu](const QPoint& pos) {
        handlePaneBlankContextMenu(pos, m_containerWidget);
    });

    auto updateSectionCountsAndHints = [this]() {
        tryPendingSelection();
        int total = m_proxyModel ? m_proxyModel->rowCount() : 0;
        int folderCount = 0;
        int fileCount = 0;
        for (int i = 0; i < total; ++i) {
            QModelIndex idx = m_proxyModel->index(i, 0);
            bool isDir = idx.data(TypeRole).toString() == "folder" || idx.data(Qt::UserRole + 2).toBool();
            if (isDir) folderCount++;
            else fileCount++;
        }

        if (m_folderHeader) {
            m_folderHeader->setCount(folderCount);
            m_folderHeader->setVisible(folderCount > 0);
        }
        if (m_fileHeader) {
            m_fileHeader->setCount(fileCount);
            m_fileHeader->setVisible(fileCount > 0 && folderCount > 0);
        }

        if (m_unifiedListView && total > 0) {
            int rowH = m_unifiedListView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 28;
            int totalH = total * rowH + 2;
            m_unifiedListView->setFixedHeight(totalH);
        }
        update();
    };

    connect(m_proxyModel, &QAbstractItemModel::modelReset, this, updateSectionCountsAndHints);
    connect(m_proxyModel, &QAbstractItemModel::layoutChanged, this, updateSectionCountsAndHints);

    connect(m_unifiedListView, &DropListView::blankSpaceClicked, this, [this]() {
        int paneIdx = property("paneIndex").toInt();
        if (m_contentPanel && m_contentPanel->columnView()) {
            m_contentPanel->columnView()->activatePaneFromBlankClick(paneIdx);
        }
    });

    connect(m_unifiedListView, &DropListView::blankSpaceDoubleClicked, this, [this]() {
        int paneIdx = property("paneIndex").toInt();
        emit blankSpaceDoubleClicked(paneIdx);
    });

    if (m_paneScrollArea && m_paneScrollArea->verticalScrollBar()) {
        connect(m_paneScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
            refreshVisibleThumbnails();
        });
    }

    m_paneScrollArea->installEventFilter(this);
    m_containerWidget->installEventFilter(this);
    m_unifiedListView->installEventFilter(this);
    if (m_unifiedListView->viewport()) m_unifiedListView->viewport()->installEventFilter(this);

    if (m_contentPanel) {
        m_unifiedListView->installEventFilter(m_contentPanel);
        m_paneScrollArea->installEventFilter(m_contentPanel);
        m_containerWidget->installEventFilter(m_contentPanel);

        connect(m_unifiedListView, &QListView::customContextMenuRequested, this, [this](const QPoint& pos) {
            emit contextMenuRequested(m_unifiedListView, pos);
            if (m_contentPanel) {
                m_contentPanel->onCustomContextMenuRequested(m_unifiedListView, pos);
            }
        });
        connect(m_unifiedListView, &DropListView::pathsDropped, this, [this](const QStringList& paths, const QModelIndex& targetIndex) {
            emit pathsDroppedSignal(paths, targetIndex, m_path);
            if (m_contentPanel) {
                m_contentPanel->onPathsDropped(paths, targetIndex, m_path, m_proxyModel);
            }
        });
    }

    if (m_unifiedListView->selectionModel()) {
        connect(m_unifiedListView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
            emit selectionChanged();
        });
    }

    connect(m_unifiedListView, &QListView::clicked, this, [this](const QModelIndex& index) {
        QString itemPath = index.data(PathRole).toString();
        bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool() || QFileInfo(itemPath).isDir();
        int paneIdx = property("paneIndex").toInt();
        if (isDir) {
            emit folderClicked(itemPath, paneIdx);
        } else {
            emit fileClicked(itemPath, paneIdx);
        }
    });

    connect(m_unifiedListView, &QListView::doubleClicked, this, [this](const QModelIndex& index) {
        if (index.isValid()) {
            QString itemPath = index.data(PathRole).toString();
            bool isDir = (index.data(TypeRole).toString() == "folder") || index.data(Qt::UserRole + 2).toBool() || QFileInfo(itemPath).isDir();
            int paneIdx = property("paneIndex").toInt();
            if (isDir) {
                emit folderExpandRequested(itemPath, paneIdx);
            } else if (m_contentPanel) {
                m_contentPanel->onDoubleClicked(index);
            }
        }
    });
}

void ColumnViewPane::setActive(bool active) {
    if (m_isActive != active) {
        m_isActive = active;
        update();
    }
}

void ColumnViewPane::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    QPainter painter(this);
    // 【架构红线 - 蓝色顶部焦点提示线设计约束】
    // 此处绘制的顶部蓝线（颜色 #3498db，宽度 1px）是"列视图（Column View）中活跃列"的焦点视觉标识。
    // ─────────────────────────────────────────────────────────────────────────────────
    // ✅ 唯一合法应用范围：ColumnViewPane（列视图中的单列窗格），当 m_isActive == true 时绘制。
    // ❌ 严禁应用范围：
    //    - ContentPanel（内容面板/窗格）     → 其活跃状态由 ContentPaneSplitManager::setActivePane
    //                                          通过 QSS property "activePane" 独立管理（border样式）。
    //    - ContentHeaderWidget（面板标题栏） → 其活跃状态由 ContentHeaderWidget::setActive
    //                                          通过 QSS property "activePane" 独立管理。
    //    - 任何其他非 ColumnViewPane 类型的 Widget。
    // ─────────────────────────────────────────────────────────────────────────────────
    // 两条"活跃"状态机（列焦点 vs 窗格焦点）完全正交，绝对不可混用或合并。
    if (m_isActive) {
        painter.setPen(QPen(QColor("#3498db"), 1));
        painter.drawLine(0, 0, width(), 0);
    }
    if (m_folderListView && m_folderListView->isVisible()) {
        int folderBottom = m_folderListView->y() + m_folderListView->height();
        if (folderBottom >= height()) {
            painter.setPen(QPen(QColor("#3498db"), 1));
            painter.drawLine(0, height() - 1, width(), height() - 1);
        }
    }
}

bool ColumnViewPane::eventFilter(QObject* obj, QEvent* event) {
    if (event && event->type() == QEvent::Wheel) {
        auto* wEvent = static_cast<QWheelEvent*>(event);
        if (!(wEvent->modifiers() & Qt::ControlModifier)) {
            if (m_paneScrollArea && m_paneScrollArea->verticalScrollBar() && m_paneScrollArea->verticalScrollBar()->isVisible()) {
                int delta = wEvent->angleDelta().y();
                if (delta == 0) delta = wEvent->pixelDelta().y();
                if (delta != 0) {
                    QScrollBar* sb = m_paneScrollArea->verticalScrollBar();
                    int step = sb->singleStep();
                    if (step <= 0) step = 20;
                    int scrollAmount = (delta / 120.0) * step * 3;
                    if (scrollAmount == 0) scrollAmount = (delta > 0 ? -step : step);
                    else scrollAmount = -scrollAmount;
                    sb->setValue(sb->value() + scrollAmount);
                    return true;
                }
            }
        }
    }

    if (event && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton && (obj == m_paneScrollArea || obj == m_panel)) {
            int paneIdx = property("paneIndex").toInt();
            if (m_contentPanel && m_contentPanel->columnView()) {
                m_contentPanel->columnView()->activatePaneFromBlankClick(paneIdx);
            }
        }
    }

    if (event && event->type() == QEvent::KeyPress && (obj == m_folderListView || obj == m_listView)) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        int paneIdx = property("paneIndex").toInt();
        if (keyEvent->key() == Qt::Key_Right || keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            DropListView* view = qobject_cast<DropListView*>(obj);
            if (view && view->currentIndex().isValid()) {
                QModelIndex idx = view->currentIndex();
                QString itemPath = idx.data(PathRole).toString();
                bool isDir = (idx.data(TypeRole).toString() == "folder") || idx.data(Qt::UserRole + 2).toBool() || QFileInfo(itemPath).isDir();
                if (isDir && !itemPath.isEmpty()) {
                    emit folderExpandRequested(itemPath, paneIdx);
                    return true;
                }
            }
        } else if (keyEvent->key() == Qt::Key_Left) {
            if (paneIdx > 0 && m_contentPanel && m_contentPanel->columnView()) {
                m_contentPanel->columnView()->focusPane(paneIdx - 1);
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

DropListView* ColumnViewPane::listView() const { return m_listView; }
DropListView* ColumnViewPane::folderListView() const { return m_folderListView; }
FolderSectionHeaderBar* ColumnViewPane::folderHeader() const { return m_panel ? m_panel->folderHeader() : nullptr; }

void ColumnViewPane::refreshVisibleThumbnails() {
    if (m_panel && m_model && m_paneScrollArea && m_paneScrollArea->viewport()) {
        m_panel->refreshVisibleThumbnails(m_model, m_paneScrollArea->viewport());
    }
}

void ColumnViewPane::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_panel && m_paneScrollArea && m_paneScrollArea->viewport()) {
        int viewportH = m_paneScrollArea->viewport()->height();
        m_panel->updateSectionCounts(viewportH);
        int folderCount = m_folderProxyModel ? m_folderProxyModel->rowCount() : 0;
        int fileCount = m_fileProxyModel ? m_fileProxyModel->rowCount() : 0;
        if (m_folderListView && folderCount > 0) {
            int rowH = m_folderListView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 28;
            int folderH = folderCount * rowH + 2;
            if (fileCount == 0) {
                m_folderListView->setFixedHeight(qMax(folderH, m_panel->folderViewMinHeight()));
            } else {
                m_folderListView->setFixedHeight(folderH);
            }
        }
        if (m_listView && fileCount > 0) {
            int rowH = m_listView->sizeHintForRow(0);
            if (rowH <= 0) rowH = 28;
            int fileH = fileCount * rowH + 2;
            m_listView->setFixedHeight(qMax(fileH, m_panel->fileViewMinHeight()));
        }
    }
    update();
}

void ColumnViewPane::setFilterState(const FilterState& state) {
    if (m_folderProxyModel) {
        FilterState s = state;
        s.showFolders = true;
        s.showFiles = false;
        m_folderProxyModel->currentFilter = s;
        m_folderProxyModel->updateFilter();
    }
    if (m_fileProxyModel) {
        FilterState s = state;
        s.showFolders = false;
        s.showFiles = true;
        m_fileProxyModel->currentFilter = s;
        m_fileProxyModel->updateFilter();
    }
}

void ColumnViewPane::selectItemByPath(const QString& targetPath) {
    m_pendingSelectPath = targetPath;
    tryPendingSelection();
}

void ColumnViewPane::setPendingSelectPaths(const QSet<QString>& paths, bool edit) {
    m_pendingSelectPaths = paths;
    m_pendingSelectPath.clear();
    m_isPendingEdit = edit;
    tryPendingSelection();
}

void ColumnViewPane::applySort(int sortType, Qt::SortOrder sortOrder) {
    if (m_folderProxyModel) {
        m_folderProxyModel->setSortType(sortType);
        m_folderProxyModel->sort(0, sortOrder);
    }
    if (m_fileProxyModel) {
        m_fileProxyModel->setSortType(sortType);
        m_fileProxyModel->sort(0, sortOrder);
    }
}

void ColumnViewPane::tryPendingSelection() {
    if (!m_fileProxyModel || !m_listView) return;

    if (!m_pendingSelectPaths.isEmpty()) {
        QSet<QString> normalizedPending;
        normalizedPending.reserve(m_pendingSelectPaths.size());
        for (const QString& p : m_pendingSelectPaths) {
            normalizedPending.insert(QDir::toNativeSeparators(QDir::cleanPath(p)).toLower());
        }

        QItemSelection fileSel;
        QModelIndex lastFileIdx;
        if (m_fileProxyModel->rowCount() > 0) {
            for (int r = 0; r < m_fileProxyModel->rowCount(); ++r) {
                QModelIndex idx = m_fileProxyModel->index(r, 0);
                QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString())).toLower();
                if (normalizedPending.contains(itemPath)) {
                    fileSel.select(idx, idx);
                    lastFileIdx = idx;
                }
            }
        }

        QItemSelection folderSel;
        QModelIndex lastFolderIdx;
        if (m_folderProxyModel && m_folderProxyModel->rowCount() > 0) {
            for (int r = 0; r < m_folderProxyModel->rowCount(); ++r) {
                QModelIndex idx = m_folderProxyModel->index(r, 0);
                QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString())).toLower();
                if (normalizedPending.contains(itemPath)) {
                    folderSel.select(idx, idx);
                    lastFolderIdx = idx;
                }
            }
        }

        bool matchedAny = false;
        DropListView* editView = nullptr;
        QModelIndex editIdx;

        if (!fileSel.isEmpty() && m_listView->selectionModel()) {
            QSignalBlocker blocker(m_listView->selectionModel());
            m_listView->selectionModel()->select(fileSel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            if (lastFileIdx.isValid()) {
                m_listView->selectionModel()->setCurrentIndex(lastFileIdx, QItemSelectionModel::NoUpdate);
                m_listView->scrollTo(lastFileIdx, QAbstractItemView::PositionAtCenter);
                editView = m_listView;
                editIdx = lastFileIdx;
            }
            matchedAny = true;
        }
        if (!folderSel.isEmpty() && m_folderListView && m_folderListView->selectionModel()) {
            QSignalBlocker blocker(m_folderListView->selectionModel());
            m_folderListView->selectionModel()->select(folderSel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            if (lastFolderIdx.isValid()) {
                m_folderListView->selectionModel()->setCurrentIndex(lastFolderIdx, QItemSelectionModel::NoUpdate);
                m_folderListView->scrollTo(lastFolderIdx, QAbstractItemView::PositionAtCenter);
                editView = m_folderListView;
                editIdx = lastFolderIdx;
            }
            matchedAny = true;
        }

        if (matchedAny) {
            m_pendingSelectPaths.clear();
            m_pendingSelectPath.clear();
            if (m_isPendingEdit && editView && editIdx.isValid()) {
                m_isPendingEdit = false;
                QPointer<DropListView> weakEditView(editView);
                QTimer::singleShot(0, this, [weakEditView, editIdx]() {
                    if (weakEditView && editIdx.isValid()) {
                        weakEditView->setFocus();
                        weakEditView->setCurrentIndex(editIdx);
                        weakEditView->edit(editIdx);
                    }
                });
            }
            emit selectionChanged();
            return;
        }
    }

    if (!m_pendingSelectPath.isEmpty()) {
        QString cleanTarget = QDir::toNativeSeparators(QDir::cleanPath(m_pendingSelectPath));
        QString targetName = QFileInfo(cleanTarget).fileName();

        if (m_folderProxyModel && m_folderListView) {
            for (int r = 0; r < m_folderProxyModel->rowCount(); ++r) {
                QModelIndex idx = m_folderProxyModel->index(r, 0);
                QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString()));
                QString itemName = QFileInfo(itemPath).fileName();

                if (QString::compare(itemPath, cleanTarget, Qt::CaseInsensitive) == 0 ||
                    (!targetName.isEmpty() && QString::compare(itemName, targetName, Qt::CaseInsensitive) == 0)) {
                    if (m_folderListView->selectionModel()) {
                        QSignalBlocker blocker(m_folderListView->selectionModel());
                        m_folderListView->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                        m_folderListView->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::NoUpdate);
                    }
                    m_folderListView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
                    m_pendingSelectPath.clear();
                    emit selectionChanged();
                    return;
                }
            }
        }

        if (m_fileProxyModel && m_listView) {
            for (int r = 0; r < m_fileProxyModel->rowCount(); ++r) {
                QModelIndex idx = m_fileProxyModel->index(r, 0);
                QString itemPath = QDir::toNativeSeparators(QDir::cleanPath(idx.data(PathRole).toString()));
                QString itemName = QFileInfo(itemPath).fileName();

                if (QString::compare(itemPath, cleanTarget, Qt::CaseInsensitive) == 0 ||
                    (!targetName.isEmpty() && QString::compare(itemName, targetName, Qt::CaseInsensitive) == 0)) {
                    if (m_listView->selectionModel()) {
                        QSignalBlocker blocker(m_listView->selectionModel());
                        m_listView->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                        m_listView->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::NoUpdate);
                    }
                    m_listView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
                    m_pendingSelectPath.clear();
                    emit selectionChanged();
                    return;
                }
            }
        }
    }
}

void ColumnViewPane::clearSelection() {
    if (m_folderListView && m_folderListView->selectionModel()) {
        QSignalBlocker blocker(m_folderListView->selectionModel());
        m_folderListView->clearSelection();
    } else if (m_folderListView) {
        m_folderListView->clearSelection();
    }

    if (m_listView && m_listView->selectionModel()) {
        QSignalBlocker blocker(m_listView->selectionModel());
        m_listView->clearSelection();
    } else if (m_listView) {
        m_listView->clearSelection();
    }
}

void ColumnViewPane::loadDirectory() {
    QString path = m_path;
    bool recursive = false;
    if (m_contentPanel && m_contentPanel->isRecursive()) {
        if (m_contentPanel->columnView() && 
            m_contentPanel->columnView()->activePane() == this) {
            recursive = true;
        }
    }
    QPointer<ColumnViewPane> weakSelf(this);
    (void)QtConcurrent::run([weakSelf, path, recursive]() {
        if (!weakSelf) return;
        std::vector<ItemRecord> items;
        if (path.isEmpty() || path == "computer://") {
            for (const QFileInfo& drive : QDir::drives()) {
                items.push_back(ItemRecord::create(drive.absolutePath()));
            }
        } else {
            items = DiskScanService::scanDirectory(path, recursive, [weakSelf]() {
                return weakSelf != nullptr;
            });
        }
        MetaCacheDecorator::decorate(items);
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakSelf, items]() {
            if (weakSelf && weakSelf->m_model) {
                weakSelf->m_model->setRecords(items);
                if (weakSelf->m_contentPanel) {
                    weakSelf->applySort(static_cast<int>(weakSelf->m_contentPanel->currentSortType()), 
                                        weakSelf->m_contentPanel->currentSortOrder());
                }
                if (!weakSelf->m_pendingSelectPaths.isEmpty()) {
                    weakSelf->tryPendingSelection();
                } else if (!weakSelf->m_pendingSelectPath.isEmpty()) {
                    weakSelf->selectItemByPath(weakSelf->m_pendingSelectPath);
                }
                int count = weakSelf->m_model->rowCount();
                if (count > 0) {
                    weakSelf->refreshVisibleThumbnails();
                }
                emit weakSelf->recordsLoaded(weakSelf->m_model->allRecords());
            }
        });
    });
}

} // namespace QuarkMeta
