# Implementation Plan - Category Binding Re-assignment & Transfer Fix

## 1. Overview
This implementation plan fixes category binding re-assignment in `LibraryDao`:
1. When dragging or adding items/paths to Category B, any existing bindings for those paths in `library_category_paths` and `library_item_index` are cleared first (`DELETE FROM library_category_paths WHERE path = ?;`, `DELETE FROM library_item_index WHERE file_path = ?;`).
2. Ensures that items moved/dragged from Category A to Category B do not remain bound to Category A.
3. Dragging to "Uncategorized" (`id == -2`) clears existing category bindings, unbinding the items from Category A.

---

## 2. Modified Files List
- `src/meta/LibraryDao.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/meta/LibraryDao.cpp`
Update `LibraryDao::addPathsToCategory` to execute previous category binding deletion before adding new category binding:

```cpp
<<<<<<< SEARCH
bool LibraryDao::addPathsToCategory(int id, const QStringList& paths) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id == 0 || paths.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    const char* sql = "INSERT OR IGNORE INTO library_category_paths (category_id, path) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    for (const QString& p : paths) {
        QString cleanP = QDir::toNativeSeparators(QDir::cleanPath(p));
        std::string pStd = cleanP.toStdString();
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_bind_text(stmt, 2, pStd.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return true;
}
=======
bool LibraryDao::addPathsToCategory(int id, const QStringList& paths) {
    sqlite3* db = DatabaseManager::instance().getGlobalDb();
    if (!db || id == 0 || paths.isEmpty()) return false;

    std::lock_guard<std::mutex> lock(DatabaseManager::instance().getGlobalMutex());

    // 1. 先清除被操作路径已有的所有旧分类关联与旧索引记录
    const char* delPathsSql = "DELETE FROM library_category_paths WHERE path = ?;";
    const char* delIndexSql = "DELETE FROM library_item_index WHERE file_path = ?;";

    sqlite3_stmt* stmtDelP = nullptr;
    sqlite3_stmt* stmtDelI = nullptr;
    if (sqlite3_prepare_v2(db, delPathsSql, -1, &stmtDelP, nullptr) == SQLITE_OK &&
        sqlite3_prepare_v2(db, delIndexSql, -1, &stmtDelI, nullptr) == SQLITE_OK) {
        for (const QString& p : paths) {
            std::string pStd = QDir::toNativeSeparators(QDir::cleanPath(p)).toStdString();

            sqlite3_bind_text(stmtDelP, 1, pStd.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmtDelP);
            sqlite3_reset(stmtDelP);

            sqlite3_bind_text(stmtDelI, 1, pStd.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmtDelI);
            sqlite3_reset(stmtDelI);
        }
    }
    if (stmtDelP) sqlite3_finalize(stmtDelP);
    if (stmtDelI) sqlite3_finalize(stmtDelI);

    // 2. 如果目标分类大于 0（即具体分类 B，而非“未分类” -2），插入新分类关联
    if (id > 0) {
        const char* sql = "INSERT OR IGNORE INTO library_category_paths (category_id, path) VALUES (?, ?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            for (const QString& p : paths) {
                std::string pStd = QDir::toNativeSeparators(QDir::cleanPath(p)).toStdString();
                sqlite3_bind_int(stmt, 1, id);
                sqlite3_bind_text(stmt, 2, pStd.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(stmt);
                sqlite3_reset(stmt);
            }
            sqlite3_finalize(stmt);
        }
    }

    sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
    return true;
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Perform CMake configuration and build.
2. Launch QuarkMeta and navigate to "库" (Library) sidebar tab.
3. Drag an item to Category A. Verify item is in Category A.
4. Drag the same item from Category A to Category B.
5. Check Category A to verify item is no longer bound to Category A, and check Category B to verify item is bound to Category B.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Modifies `LibraryDao::addPathsToCategory` SSOT database method, maintaining unified data persistence.

---

## 6. Header API Signature Verification
- `static bool LibraryDao::addPathsToCategory(int id, const QStringList& paths)`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `LibraryDao.cpp` includes `"LibraryDao.h"`, `"DatabaseManager.h"`, `<QDir>`.
