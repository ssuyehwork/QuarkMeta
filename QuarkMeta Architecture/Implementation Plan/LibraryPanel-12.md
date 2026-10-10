# Implementation Plan - LibraryPanel Recursive Ancestor Preset Tags Inheritance

## 1. Overview
This implementation plan updates `LibraryPanel::onPathsDroppedToCategory`:
1. Collects preset tags from the target category as well as all its ancestor categories recursively up the parent node chain.
2. Applies the combined ancestor preset tags to dropped files.

---

## 2. Modified Files List
- `src/ui/LibraryPanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/LibraryPanel.cpp`
Update preset tags gathering in `LibraryPanel::onPathsDroppedToCategory`:

```cpp
<<<<<<< SEARCH
        // 自动将预设标签批量加至入库文件
        auto categories = LibraryDao::getAllCategories();
        QStringList presetTags;
        for (const auto& cat : categories) {
            if (cat.id == nodeId) {
                presetTags = cat.presetTags;
                break;
            }
        }
=======
        // 自动将当前分类及其所有祖先分类的预设标签递归合并加至入库文件
        auto categories = LibraryDao::getAllCategories();
        QMap<int, LibraryCategoryRecord> catMap;
        for (const auto& cat : categories) {
            catMap.insert(cat.id, cat);
        }

        QStringList presetTags;
        int currentId = nodeId;
        while (currentId > 0 && catMap.contains(currentId)) {
            const auto& curCat = catMap.value(currentId);
            for (const QString& tag : curCat.presetTags) {
                if (!presetTags.contains(tag)) {
                    presetTags.append(tag);
                }
            }
            currentId = curCat.parentId;
        }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Perform CMake configuration and build.
2. Set preset tags on Parent Category A (e.g., `"ProjectA"`).
3. Drop an item into Subcategory A-1. Verify that the item receives `"ProjectA"` preset tag as well as any subcategory preset tags.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Reuses `CoreEngine::instance().executeCommand(AppCommandType::AddTag)`.

---

## 6. Header API Signature Verification
- `void LibraryPanel::onPathsDroppedToCategory(const QStringList& paths, const QModelIndex& target)`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `LibraryPanel.cpp` includes `"LibraryDao.h"`, `"LibraryService.h"`.
