# Implementation Plan - SearchController Clearing Stale Keyword Filter Fix

## 1. Overview
This implementation plan fixes an issue in `SearchController::doSearch` where clearing the search box text in `SearchScope::Library` mode failed to clear the active filter keyword (`m_currentFilter.keyword`) on `ContentPanel`, leaving stale filtered search results visible on the view after clearing search text.

Key objective:
- Unconditionally invoke `m_contentPanel->search(keyword)` at the beginning of `SearchController::doSearch` to synchronize and clear `ContentPanel`'s internal search keyword filter when `keyword` is empty.

---

## 2. Modified Files List
- `src/ui/SearchController.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/SearchController.cpp`
Update `SearchController::doSearch` to clear `ContentPanel` search keyword before refreshing:

```cpp
<<<<<<< SEARCH
void SearchController::doSearch(const QString& keyword) {
    if (!m_contentPanel) return;

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
    } else {
        m_contentPanel->search(keyword);
    }
=======
void SearchController::doSearch(const QString& keyword) {
    if (!m_contentPanel) return;

    // 1. 优先同步与重置 ContentPanel 内部的搜索关键词，消除残存过滤词
    m_contentPanel->search(keyword);

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
2. Select "搜索：库" in the search bar and type a search query (e.g. `BAT`). Verify matching items are displayed.
3. Clear the search bar text. Verify that stale filtered search results disappear and the original view content is completely restored without residual search filters.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `m_contentPanel->search(keyword)` and `m_contentPanel->refreshAll()` SSOT APIs.

---

## 6. Header API Signature Verification
- `void SearchController::doSearch(const QString& keyword)`
- `void ContentPanel::search(const QString& query)`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `SearchController.cpp` includes `"ContentPanel.h"`.
