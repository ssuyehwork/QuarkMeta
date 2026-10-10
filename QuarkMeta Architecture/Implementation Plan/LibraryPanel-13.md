# Implementation Plan - Removing Untagged System Category Node

## 1. Overview
This implementation plan completely removes the "未标签" ("Untagged", ID -3) system category item and its associated query logic:
1. Removes `addSystemItem("未标签", "untagged", "#7f8c8d", -3);` from `LibraryPanel::loadLibrary`.
2. Removes the `else if (id == -3)` branch from `LibraryDao::getCategoryPaths`.

---

## 2. Modified Files List
- `src/ui/LibraryPanel.cpp`
- `src/meta/LibraryDao.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/LibraryPanel.cpp`
Remove "未标签" system item creation:

```cpp
<<<<<<< SEARCH
    addSystemItem("全部数据", "all_data", "#3498db", -1);
    addSystemItem("未分类", "uncategorized", "#95a5a6", -2);
    addSystemItem("未标签", "untagged", "#7f8c8d", -3);
=======
    addSystemItem("全部数据", "all_data", "#3498db", -1);
    addSystemItem("未分类", "uncategorized", "#95a5a6", -2);
>>>>>>> REPLACE
```

### 3.2 `src/meta/LibraryDao.cpp`
Remove `id == -3` query branch:

```cpp
<<<<<<< SEARCH
    } else if (id == -3) {
        // 未标签：获取 library_item_index 中 tags 为空/NULL 的文件路径
        const char* sql = "SELECT DISTINCT file_path FROM library_item_index WHERE tags IS NULL OR tags = '';";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* pStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (pStr) paths.append(QString::fromUtf8(pStr));
            }
            sqlite3_finalize(stmt);
        }
    } else {
=======
    } else {
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Perform CMake configuration and build.
2. Launch QuarkMeta and switch to "库" (Library) sidebar tab.
3. Verify that only "全部数据" and "未分类" system items are rendered, and "未标签" item is removed.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Removes legacy/unneeded system item without breaking existing category IDs.

---

## 6. Header API Signature Verification
- `void LibraryPanel::loadLibrary()`
- `static QStringList LibraryDao::getCategoryPaths(int id)`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `LibraryPanel.cpp` includes `"LibraryDao.h"`.
