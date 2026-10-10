# Implementation Plan - LibraryPanel Category Creation & Inline Editing Fix

## 1. Overview
This implementation plan fixes category creation inline editing in `LibraryPanel`:
1. Fixes race condition where async `libraryChanged` signal triggers a second `loadLibrary()` invocation that clears the model (`m_model->clear()`) and destroys the inline `QLineEdit` editor immediately after creation.
2. Replaces top-level-only single-level loop (`for (int i = 0; i < m_model->rowCount(); ++i)`) with recursive `findItemByNodeId` helper that traverses the entire tree to locate both top-level categories and subcategories at any depth.
3. Automatically expands parent category nodes before calling `m_treeView->edit(...)` to ensure subcategory items are visible and editable.

---

## 2. Modified Files List
- `src/ui/LibraryPanel.h`
- `src/ui/LibraryPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/LibraryPanel.h`
Declare `findItemByNodeId` helper function in `LibraryPanel` private section:

```cpp
<<<<<<< SEARCH
private:
    void initUi();
    void createAndEditCategory(int parentId = 0);

    QVBoxLayout* m_mainLayout = nullptr;
=======
private:
    void initUi();
    void createAndEditCategory(int parentId = 0);
    QStandardItem* findItemByNodeId(QStandardItem* parent, int nodeId);

    QVBoxLayout* m_mainLayout = nullptr;
>>>>>>> REPLACE
```

### 3.2 `src/ui/LibraryPanel.cpp`
Implement `findItemByNodeId` and update `loadLibrary()` to defer inline editor triggering via `QTimer::singleShot(0)`:

```cpp
<<<<<<< SEARCH
    if (m_pendingEditNodeId > 0 && m_treeView) {
        int targetNodeId = m_pendingEditNodeId;
        m_pendingEditNodeId = 0;

        for (int i = 0; i < m_model->rowCount(); ++i) {
            QStandardItem* item = m_model->item(i);
            if (item && item->data(Qt::UserRole + 1).toInt() == targetNodeId) {
                m_treeView->setCurrentIndex(item->index());
                m_treeView->edit(item->index());
                break;
            }
        }
    }
}

void LibraryPanel::createAndEditCategory(int parentId) {
=======
    if (m_pendingEditNodeId > 0 && m_treeView) {
        int targetNodeId = m_pendingEditNodeId;
        QTimer::singleShot(0, this, [this, targetNodeId]() {
            if (m_pendingEditNodeId != targetNodeId) return;
            m_pendingEditNodeId = 0;

            QStandardItem* targetItem = findItemByNodeId(m_model->invisibleRootItem(), targetNodeId);
            if (targetItem && m_treeView) {
                QModelIndex parentIdx = targetItem->parent() ? targetItem->parent()->index() : QModelIndex();
                if (parentIdx.isValid()) {
                    m_treeView->expand(parentIdx);
                }
                m_treeView->setCurrentIndex(targetItem->index());
                m_treeView->edit(targetItem->index());
            }
        });
    }
}

QStandardItem* LibraryPanel::findItemByNodeId(QStandardItem* parent, int nodeId) {
    if (!parent) return nullptr;
    for (int i = 0; i < parent->rowCount(); ++i) {
        QStandardItem* child = parent->child(i);
        if (!child) continue;
        if (child->data(Qt::UserRole + 1).toInt() == nodeId) {
            return child;
        }
        QStandardItem* found = findItemByNodeId(child, nodeId);
        if (found) return found;
    }
    return nullptr;
}

void LibraryPanel::createAndEditCategory(int parentId) {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Perform CMake configuration and build.
2. Launch QuarkMeta and navigate to "库" (Library) sidebar tab.
3. Right-click on empty area and select "新建库分类". Verify that top-level category is created and inline rename editor (`QLineEdit`) is automatically opened and stays active.
4. Right-click on newly created category and select "新建子分类". Verify that parent category automatically expands, subcategory is created, and inline rename editor is automatically opened.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `LibraryService::instance().createCategory` and `loadLibrary()`.
- Unifies inline editor triggering for both top-level categories and subcategories.

---

## 6. Header API Signature Verification
- `QStandardItem* LibraryPanel::findItemByNodeId(QStandardItem* parent, int nodeId)`
- `void LibraryPanel::loadLibrary()`
- `void LibraryPanel::createAndEditCategory(int parentId = 0)`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `LibraryPanel.h` includes `<QStandardItemModel>`, `<QTreeView>`, `<QFrame>`.
- `LibraryPanel.cpp` includes `<QTimer>` (provided by `DropTreeView.h` / `<QTimer>`).
