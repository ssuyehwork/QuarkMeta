# Implementation Plan - LibraryDao Recursive Child Category Aggregation, Cascade Cleanup, & Auto-Healing

## 1. Overview
This implementation plan enhances `LibraryDao`:
1. `LibraryDao::removeCategoryById` executes recursive DELETE CTE queries on `library_category_paths` and `library_item_index` for all child category IDs prior to deleting categories from `library_categories`.
2. `LibraryDao::getCategoryPaths` uses a `WITH RECURSIVE` CTE query to aggregate paths for regular user categories (`id > 0`) together with all their nested subcategories, and verifies path existence via `QFileInfo::exists` while auto-healing non-existent dead paths.

---

## 2. Modified Files List
- `src/meta/LibraryDao.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/meta/LibraryDao.cpp`
Update `LibraryDao::removeCategoryById` to clean category paths and indexed metadata recursively for child categories:

```cpp
<<<<<<< SEARCH
    const char* sqlClean = "DELETE FROM library_category_paths WHERE category_id = ?;";
    if (sqlite3_prepare_v2(db, sqlClean, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
=======
    const char* sqlCleanPaths = "DELETE FROM library_category_paths WHERE category_id IN ("
                                "  WITH RECURSIVE cnt(x) AS ("
                                "    SELECT ? UNION ALL SELECT id FROM library_categories, cnt WHERE library_categories.parent_id = cnt.x"
                                "  ) SELECT x FROM cnt"
                                ");";
    if (sqlite3_prepare_v2(db, sqlCleanPaths, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    const char* sqlCleanIndex = "DELETE FROM library_item_index WHERE category_id IN ("
                                "  WITH RECURSIVE cnt(x) AS ("
                                "    SELECT ? UNION ALL SELECT id FROM library_categories, cnt WHERE library_categories.parent_id = cnt.x"
                                "  ) SELECT x FROM cnt"
                                ");";
    if (sqlite3_prepare_v2(db, sqlCleanIndex, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
>>>>>>> REPLACE
```

Update `LibraryDao::getCategoryPaths` for user categories (`id > 0`) to recursively query subcategories and filter/auto-heal non-existent paths:

```cpp
<<<<<<< SEARCH
    } else {
        // 常规用户分类 ID > 0
        const char* sql = "SELECT path FROM library_category_paths WHERE category_id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return paths;

        sqlite3_bind_int(stmt, 1, id);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (pStr) paths.append(QString::fromUtf8(pStr));
        }
        sqlite3_finalize(stmt);
    }
=======
    } else {
        // 常规用户分类 ID > 0：递归获取当前分类及其所有下级子分类绑定关联的所有路径
        const char* sql = "WITH RECURSIVE cat_tree(x) AS ("
                          "  SELECT ? UNION ALL SELECT id FROM library_categories, cat_tree WHERE library_categories.parent_id = cat_tree.x"
                          ") SELECT DISTINCT path FROM library_category_paths WHERE category_id IN (SELECT x FROM cat_tree);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return paths;

        sqlite3_bind_int(stmt, 1, id);
        QStringList invalidPaths;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (pStr) {
                QString path = QString::fromUtf8(pStr);
                if (QFileInfo::exists(path)) {
                    paths.append(path);
                } else {
                    invalidPaths.append(path);
                }
            }
        }
        sqlite3_finalize(stmt);

        // 自动自愈：擦除磁盘上已不存在的无效路径
        if (!invalidPaths.isEmpty()) {
            const char* delSql = "DELETE FROM library_category_paths WHERE path = ?;";
            if (sqlite3_prepare_v2(db, delSql, -1, &stmt, nullptr) == SQLITE_OK) {
                for (const QString& invP : invalidPaths) {
                    std::string pStd = invP.toStdString();
                    sqlite3_bind_text(stmt, 1, pStd.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_step(stmt);
                    sqlite3_reset(stmt);
                }
                sqlite3_finalize(stmt);
            }
        }
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Perform CMake configuration and build.
2. Create parent Category A and child Category A-1. Assign items to A-1.
3. Click Category A to verify that items in A-1 are displayed.
4. Delete Category A to verify that subcategory paths and indexes are properly cleared without leaving orphan records.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `LibraryDao` methods directly as SSOT.

---

## 6. Header API Signature Verification
- `static bool LibraryDao::removeCategoryById(int id)`
- `static QStringList LibraryDao::getCategoryPaths(int id)`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `LibraryDao.cpp` includes `"LibraryDao.h"`, `<QFileInfo>`.
