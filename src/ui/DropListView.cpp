#include "DropListView.h"
#include "../core/ModelContract.h"
#include "ColumnItemDelegate.h"
#include "ViewDragDropHelper.h"
#include "models/GroupingProxyModel.h"
#include <QMouseEvent>

namespace QuarkMeta {

DropListView::DropListView(QWidget* parent) : QTreeView(parent) {
    setHeaderHidden(true);
    setIndentation(0);
    setRootIsDecorated(true);
    setDragEnabled(true);
    setAcceptDrops(true);

    connect(this, &QTreeView::collapsed, this, [this](const QModelIndex& index) {
        if (index.data(GroupingProxyModel::IsGroupHeaderRole).toBool()) {
            QString groupId = index.data(GroupingProxyModel::GroupIdRole).toString();
            bool isCollapsible = index.data(GroupingProxyModel::GroupIsCollapsibleRole).toBool();
            if (!isCollapsible) {
                setExpanded(index, true);
            } else if (auto* groupModel = qobject_cast<GroupingProxyModel*>(model())) {
                groupModel->setGroupCollapsed(groupId, true);
            }
        }
    });

    connect(this, &QTreeView::expanded, this, [this](const QModelIndex& index) {
        if (index.data(GroupingProxyModel::IsGroupHeaderRole).toBool()) {
            QString groupId = index.data(GroupingProxyModel::GroupIdRole).toString();
            if (auto* groupModel = qobject_cast<GroupingProxyModel*>(model())) {
                groupModel->setGroupCollapsed(groupId, false);
            }
        }
    });
}

void DropListView::setModel(QAbstractItemModel* model) {
    QTreeView::setModel(model);
    if (auto* groupModel = qobject_cast<GroupingProxyModel*>(model)) {
        connect(groupModel, &QAbstractItemModel::modelReset, this, [this, groupModel]() {
            for (int r = 0; r < groupModel->rowCount(); ++r) {
                QModelIndex groupIdx = groupModel->index(r, 0);
                QString groupId = groupIdx.data(GroupingProxyModel::GroupIdRole).toString();
                bool isCollapsed = groupModel->isGroupCollapsed(groupId);
                bool isCollapsible = groupIdx.data(GroupingProxyModel::GroupIsCollapsibleRole).toBool();

                if (!isCollapsible || !isCollapsed) {
                    setExpanded(groupIdx, true);
                } else {
                    setExpanded(groupIdx, false);
                }
            }
        });
    }
}

void DropListView::dragEnterEvent(QDragEnterEvent* event) {
    if (!ViewDragDropHelper::handleDragEnter(this, event)) {
        QTreeView::dragEnterEvent(event);
    }
}

void DropListView::dragMoveEvent(QDragMoveEvent* event) {
    QModelIndex hoverIdx = indexAt(event->position().toPoint());
    if (m_currentHoverDropIdx != hoverIdx) {
        clearDropHighlight();
        if (hoverIdx.isValid()) {
            bool isFolder = (hoverIdx.data(TypeRole).toString() == "folder") || hoverIdx.data(Qt::UserRole + 2).toBool();
            if (isFolder) {
                m_currentHoverDropIdx = hoverIdx;
                if (model()) {
                    const_cast<QAbstractItemModel*>(model())->setData(m_currentHoverDropIdx, true, IsDropTargetRole);
                    viewport()->update();
                }
            }
        }
    }

    if (!ViewDragDropHelper::handleDragMove(this, event)) {
        QTreeView::dragMoveEvent(event);
    }
}

void DropListView::dragLeaveEvent(QDragLeaveEvent* event) {
    clearDropHighlight();
    ViewDragDropHelper::clearHover(this);
    QTreeView::dragLeaveEvent(event);
}

void DropListView::clearDropHighlight() {
    if (m_currentHoverDropIdx.isValid() && model()) {
        const_cast<QAbstractItemModel*>(model())->setData(m_currentHoverDropIdx, false, IsDropTargetRole);
        m_currentHoverDropIdx = QModelIndex();
        viewport()->update();
    }
}

void DropListView::dropEvent(QDropEvent* event) {
    clearDropHighlight();
    QStringList paths;
    QModelIndex targetIdx;
    if (ViewDragDropHelper::handleDrop(this, event, paths, targetIdx)) {
        emit pathsDropped(paths, targetIdx);
    } else {
        QTreeView::dropEvent(event);
    }
}

void DropListView::startDrag(Qt::DropActions supportedActions) {
    ViewDragDropHelper::executeStartDrag(this, supportedActions);
}

void DropListView::mousePressEvent(QMouseEvent* event) {
    QTreeView::mousePressEvent(event);
}

void DropListView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event && event->button() == Qt::LeftButton) {
        QModelIndex idx = indexAt(event->pos());
        if (!idx.isValid()) {
            emit blankSpaceDoubleClicked();
            event->accept();
            return;
        }
    }
    QTreeView::mouseDoubleClickEvent(event);
}

} // namespace QuarkMeta
