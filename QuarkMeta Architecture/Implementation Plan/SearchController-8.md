# Implementation Plan - SearchController Full Library Search Scope Coverage

## 1. Overview
This implementation plan updates `SearchController::doSearch`:
1. When `m_searchScope == SearchScope::Library`, queries `LibraryDao::getCategoryPaths(-1)` to collect all valid library paths (including regular user categories, uncategorized items, and indexed files) for global library searching.

---

## 2. Modified Files List
- `src/ui/SearchController.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/SearchController.cpp`
Update `SearchController::doSearch` for `SearchScope::Library`:

```cpp
<<<<<<< SEARCH
    if (m_searchScope == SearchScope::Library) {
        if (keyword.isEmpty()) {
            m_contentPanel->refreshAll();
        } else {
            LibraryDao::initTable();
            auto categories = LibraryDao::getAllCategories();
            QStringList allLibraryPaths;
            for (const auto& cat : categories) {
                allLibraryPaths.append(cat.associatedPaths);
            }
            allLibraryPaths.removeDuplicates();

            m_contentPanel->loadPaths(allLibraryPaths);
            m_contentPanel->search(keyword);
        }
    }
=======
    if (m_searchScope == SearchScope::Library) {
        if (keyword.isEmpty()) {
            m_contentPanel->refreshAll();
        } else {
            LibraryDao::initTable();
            QStringList allLibraryPaths = LibraryDao::getCategoryPaths(-1);
            allLibraryPaths.removeDuplicates();

            m_contentPanel->loadPaths(allLibraryPaths);
            m_contentPanel->search(keyword);
        }
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Perform CMake configuration and build.
2. Select "搜索：库" in the search bar.
3. Search for a file that is in "Uncategorized" or indexed in the library. Verify that search results include the matching file.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `LibraryDao::getCategoryPaths(-1)` SSOT query.

---

## 6. Header API Signature Verification
- `void SearchController::doSearch(const QString& keyword)`
- `static QStringList LibraryDao::getCategoryPaths(int id)`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `SearchController.cpp` includes `"../meta/LibraryDao.h"`.
