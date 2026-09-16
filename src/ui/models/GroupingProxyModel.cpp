#include "GroupingProxyModel.h"

namespace QuarkMeta {

GroupingProxyModel::GroupingProxyModel(QObject* parent)
    : QAbstractProxyModel(parent)
{
    setupDefaultGroups();
}

void GroupingProxyModel::setupDefaultGroups() {
    m_groupDefs.clear();

    GroupDefinition folderGroup;
    folderGroup.id = "folders";
    folderGroup.titleTemplate = QString::fromUtf8("文件夹 (%1)");
    folderGroup.isCollapsible = true;
    folderGroup.matchPredicate = [](const QModelIndex& srcIdx) {
        return srcIdx.data(TypeRole).toString() == "folder";
    };

    GroupDefinition fileGroup;
    fileGroup.id = "files";
    fileGroup.titleTemplate = QString::fromUtf8("文件 (%1)");
    fileGroup.isCollapsible = false;
    fileGroup.matchPredicate = [](const QModelIndex& srcIdx) {
        return srcIdx.data(TypeRole).toString() != "folder";
    };

    m_groupDefs.append(folderGroup);
    m_groupDefs.append(fileGroup);
}

void GroupingProxyModel::setSourceModel(QAbstractItemModel* newSourceModel) {
    if (sourceModel()) {
        disconnect(sourceModel(), &QAbstractItemModel::dataChanged, this, &GroupingProxyModel::onSourceDataChanged);
        disconnect(sourceModel(), &QAbstractItemModel::modelReset, this, &GroupingProxyModel::onSourceReset);
        disconnect(sourceModel(), &QAbstractItemModel::layoutChanged, this, &GroupingProxyModel::onSourceReset);
        disconnect(sourceModel(), &QAbstractItemModel::rowsInserted, this, &GroupingProxyModel::onSourceReset);
        disconnect(sourceModel(), &QAbstractItemModel::rowsRemoved, this, &GroupingProxyModel::onSourceReset);
    }

    QAbstractProxyModel::setSourceModel(newSourceModel);

    if (newSourceModel) {
        connect(newSourceModel, &QAbstractItemModel::dataChanged, this, &GroupingProxyModel::onSourceDataChanged);
        connect(newSourceModel, &QAbstractItemModel::modelReset, this, &GroupingProxyModel::onSourceReset);
        connect(newSourceModel, &QAbstractItemModel::layoutChanged, this, &GroupingProxyModel::onSourceReset);
        connect(newSourceModel, &QAbstractItemModel::rowsInserted, this, &GroupingProxyModel::onSourceReset);
        connect(newSourceModel, &QAbstractItemModel::rowsRemoved, this, &GroupingProxyModel::onSourceReset);
    }

    rebuildMapping();
}

void GroupingProxyModel::onSourceReset() {
    rebuildMapping();
}

void GroupingProxyModel::onSourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles) {
    if (!topLeft.isValid() || !bottomRight.isValid()) {
        rebuildMapping();
        return;
    }
    QModelIndex proxyTopLeft = mapFromSource(topLeft);
    QModelIndex proxyBottomRight = mapFromSource(bottomRight);
    if (proxyTopLeft.isValid() && proxyBottomRight.isValid()) {
        emit dataChanged(proxyTopLeft, proxyBottomRight, roles);
    } else {
        rebuildMapping();
    }
}

void GroupingProxyModel::rebuildMapping() {
    beginResetModel();
    m_activeGroups.clear();

    if (!sourceModel()) {
        endResetModel();
        return;
    }

    int totalSourceRows = sourceModel()->rowCount();
    for (const auto& groupDef : m_groupDefs) {
        GroupNode groupNode;
        groupNode.definition = groupDef;

        for (int r = 0; r < totalSourceRows; ++r) {
            QModelIndex srcIdx = sourceModel()->index(r, 0);
            if (groupDef.matchPredicate && groupDef.matchPredicate(srcIdx)) {
                groupNode.sourceRows.append(r);
            }
        }

        if (!groupNode.sourceRows.isEmpty()) {
            m_activeGroups.append(groupNode);
        }
    }

    endResetModel();
}

QModelIndex GroupingProxyModel::mapToSource(const QModelIndex& proxyIndex) const {
    if (!proxyIndex.isValid()) return QModelIndex();

    quintptr internalId = proxyIndex.internalId();
    if (internalId == 0) {
        // Top-level group node has no direct source index mapping
        return QModelIndex();
    }

    int groupIdx = static_cast<int>(internalId) - 1;
    if (groupIdx < 0 || groupIdx >= m_activeGroups.size()) return QModelIndex();

    const auto& node = m_activeGroups[groupIdx];
    int childRow = proxyIndex.row();
    if (childRow < 0 || childRow >= node.sourceRows.size()) return QModelIndex();

    int srcRow = node.sourceRows[childRow];
    return sourceModel() ? sourceModel()->index(srcRow, proxyIndex.column()) : QModelIndex();
}

QModelIndex GroupingProxyModel::mapFromSource(const QModelIndex& sourceIndex) const {
    if (!sourceIndex.isValid() || !sourceModel()) return QModelIndex();

    int srcRow = sourceIndex.row();
    for (int g = 0; g < m_activeGroups.size(); ++g) {
        const auto& node = m_activeGroups[g];
        for (int c = 0; c < node.sourceRows.size(); ++c) {
            if (node.sourceRows[c] == srcRow) {
                return createIndex(c, sourceIndex.column(), static_cast<quintptr>(g + 1));
            }
        }
    }
    return QModelIndex();
}

QModelIndex GroupingProxyModel::index(int row, int column, const QModelIndex& parent) const {
    if (row < 0 || column < 0 || column >= columnCount()) return QModelIndex();

    if (!parent.isValid()) {
        // Top-level group index
        if (row >= m_activeGroups.size()) return QModelIndex();
        return createIndex(row, column, static_cast<quintptr>(0));
    }

    if (parent.internalId() == 0) {
        // Child under group node
        int groupIdx = parent.row();
        if (groupIdx < 0 || groupIdx >= m_activeGroups.size()) return QModelIndex();
        if (row >= m_activeGroups[groupIdx].sourceRows.size()) return QModelIndex();
        return createIndex(row, column, static_cast<quintptr>(groupIdx + 1));
    }

    return QModelIndex();
}

QModelIndex GroupingProxyModel::parent(const QModelIndex& child) const {
    if (!child.isValid()) return QModelIndex();

    quintptr internalId = child.internalId();
    if (internalId == 0) {
        return QModelIndex();
    }

    int groupIdx = static_cast<int>(internalId) - 1;
    if (groupIdx >= 0 && groupIdx < m_activeGroups.size()) {
        return createIndex(groupIdx, 0, static_cast<quintptr>(0));
    }

    return QModelIndex();
}

int GroupingProxyModel::rowCount(const QModelIndex& parent) const {
    if (!parent.isValid()) {
        return m_activeGroups.size();
    }
    if (parent.internalId() == 0) {
        int groupIdx = parent.row();
        if (groupIdx >= 0 && groupIdx < m_activeGroups.size()) {
            return m_activeGroups[groupIdx].sourceRows.size();
        }
    }
    return 0;
}

int GroupingProxyModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return sourceModel() ? sourceModel()->columnCount() : 0;
}

QVariant GroupingProxyModel::data(const QModelIndex& proxyIndex, int role) const {
    if (!proxyIndex.isValid()) return QVariant();

    if (proxyIndex.internalId() == 0) {
        // Top-level group node data
        int groupIdx = proxyIndex.row();
        if (groupIdx < 0 || groupIdx >= m_activeGroups.size()) return QVariant();

        const auto& node = m_activeGroups[groupIdx];
        const auto& groupDef = node.definition;
        int count = node.sourceRows.size();

        switch (role) {
            case GroupRole::IsGroupHeaderRole:
                return true;
            case GroupRole::GroupIdRole:
                return groupDef.id;
            case GroupRole::GroupTitleRole:
                return groupDef.titleTemplate.arg(count);
            case GroupRole::GroupIsCollapsibleRole:
                return groupDef.isCollapsible;
            case GroupRole::GroupIsCollapsedRole:
                return isGroupCollapsed(groupDef.id);
            case GroupRole::GroupItemCountRole:
                return count;
            case Qt::DisplayRole:
                return (proxyIndex.column() == 0) ? groupDef.titleTemplate.arg(count) : QVariant();
            default:
                return QVariant();
        }
    }

    if (role == GroupRole::IsGroupHeaderRole) {
        return false;
    }

    QModelIndex srcIdx = mapToSource(proxyIndex);
    return srcIdx.isValid() ? srcIdx.data(role) : QVariant();
}

Qt::ItemFlags GroupingProxyModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return Qt::NoItemFlags;
    if (index.internalId() == 0) {
        // Group nodes are enabled but NOT selectable
        return Qt::ItemIsEnabled;
    }
    QModelIndex srcIdx = mapToSource(index);
    return srcIdx.isValid() ? srcIdx.flags() : Qt::NoItemFlags;
}

bool GroupingProxyModel::isGroupHeader(const QModelIndex& index) const {
    return index.isValid() && (index.internalId() == 0);
}

bool GroupingProxyModel::setGroupCollapsed(const QString& groupId, bool collapsed) {
    for (const auto& group : m_groupDefs) {
        if (group.id == groupId) {
            if (!group.isCollapsible) return false;
            if (collapsed) {
                m_collapsedGroupIds.insert(groupId);
            } else {
                m_collapsedGroupIds.remove(groupId);
            }
            return true;
        }
    }
    return false;
}

bool GroupingProxyModel::isGroupCollapsed(const QString& groupId) const {
    return m_collapsedGroupIds.contains(groupId);
}

QModelIndex GroupingProxyModel::groupHeaderIndex(const QString& groupId) const {
    for (int i = 0; i < m_activeGroups.size(); ++i) {
        if (m_activeGroups[i].definition.id == groupId) {
            return createIndex(i, 0, static_cast<quintptr>(0));
        }
    }
    return QModelIndex();
}

} // namespace QuarkMeta
