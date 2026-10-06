# ContentHeaderWidget Implementation Plan - Dynamic Header Title for Multi-Pane View

This implementation plan outlines the structural refactoring to make the Content Panel header title dynamic:
- Display "内容" when operating in single-pane mode.
- Display the clean folder/directory name (e.g., "测试", "QuarkMeta_backups", "此电脑", "回收站") when operating in multi-pane split mode.

---

## 1. Overview
Currently, `ContentHeaderWidget` initializes its title label with a fixed string `"内容"` (`m_titleLabel = new QLabel("内容", this)`).
When users enter multi-pane split view (2 to 4 split panes), having every pane header title display `"内容"` provides no visual distinction between panes.

To improve user clarity and navigation context:
1. `ContentHeaderWidget` will expose a public API `void setTitle(const QString& title)`.
2. `ContentPanel` will maintain an internal helper `updateHeaderTitle()`:
   - When `isSplitMode()` is `false`, sets title to `"内容"`.
   - When `isSplitMode()` is `true`, formats `m_currentPath` into a user-friendly display name ("此电脑" for empty or `computer://`, "回收站" for `trash://`, drive root or folder name for disk paths) and sets it via `m_headerWidget->setTitle(...)`.
3. `ContentPaneSplitManager` will trigger header title updates for all active/inactive panes whenever layout changes occur (`notifyLayoutChanged` / `updateFocusBorders`).

---

## 2. Modified Files List
- `src/ui/ContentHeaderWidget.h`
- `src/ui/ContentHeaderWidget.cpp`
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `src/ui/controllers/ContentPaneSplitManager.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/ContentHeaderWidget.h`

```
<<<<<<< SEARCH
    void setFilterState(const FilterState& state);
    void setRecursive(bool recursive);
    void setLayersEnabled(bool enabled, const QString& tooltip);
    void setActive(bool active);
=======
    void setTitle(const QString& title);
    void setFilterState(const FilterState& state);
    void setRecursive(bool recursive);
    void setLayersEnabled(bool enabled, const QString& tooltip);
    void setActive(bool active);
>>>>>>> REPLACE
```

### 3.2 `src/ui/ContentHeaderWidget.cpp`

```
<<<<<<< SEARCH
void ContentHeaderWidget::setFilterState(const FilterState& state) {
=======
void ContentHeaderWidget::setTitle(const QString& title) {
    if (m_titleLabel) {
        m_titleLabel->setText(title.isEmpty() ? "内容" : title);
    }
}

void ContentHeaderWidget::setFilterState(const FilterState& state) {
>>>>>>> REPLACE
```

### 3.3 `src/ui/ContentPanel.h`

```
<<<<<<< SEARCH
    class ContentHeaderWidget* m_headerWidget = nullptr;
=======
    void updateHeaderTitle();

    class ContentHeaderWidget* m_headerWidget = nullptr;
>>>>>>> REPLACE
```

### 3.4 `src/ui/ContentPanel.cpp`

```
<<<<<<< SEARCH
#include <QFileInfo>
#include <QMenu>
=======
#include <QFileInfo>
#include <QDir>
#include <QMenu>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::setCurrentPath(const QString& path) {
    m_currentPath = path;
    if (m_model) {
        m_model->setCurrentPath(path);
    }
    ContentPanel* root = rootPane();
    if (root && root->splitManager()) {
        emit root->splitManager()->layoutChanged();
    }
}
=======
void ContentPanel::updateHeaderTitle() {
    if (!m_headerWidget) return;
    if (!isSplitMode()) {
        m_headerWidget->setTitle("内容");
        return;
    }

    if (m_currentPath.isEmpty() || m_currentPath == "computer://") {
        m_headerWidget->setTitle("此电脑");
        return;
    }
    if (m_currentPath == "trash://") {
        m_headerWidget->setTitle("回收站");
        return;
    }

    QString cleanP = QDir::cleanPath(m_currentPath);
    QFileInfo fi(cleanP);
    if (fi.isRoot() || cleanP.endsWith(":\\") || cleanP.endsWith(":/") || (cleanP.length() == 2 && cleanP.endsWith(':'))) {
        m_headerWidget->setTitle(cleanP);
        return;
    }

    QString folderName = fi.fileName();
    m_headerWidget->setTitle(folderName.isEmpty() ? cleanP : folderName);
}

void ContentPanel::setCurrentPath(const QString& path) {
    m_currentPath = path;
    if (m_model) {
        m_model->setCurrentPath(path);
    }
    updateHeaderTitle();
    ContentPanel* root = rootPane();
    if (root && root->splitManager()) {
        emit root->splitManager()->layoutChanged();
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
=======
void ContentPanel::loadDirectory(const QString& path, bool recursive) {
    updateHeaderTitle();
>>>>>>> REPLACE
```

### 3.5 `src/ui/controllers/ContentPaneSplitManager.cpp`

```
<<<<<<< SEARCH
    if (m_panel && m_panel->m_headerWidget) {
        m_panel->m_headerWidget->setActive(rootShown);
    }

    // 副窗格激活判定
    for (ContentPanel* pane : m_panes) {
        if (!pane) continue;
        bool paneActive = (m_activePaneForSplit == pane);
        bool paneShown = isSplit && paneActive;

        setPaneActiveProperty(pane, paneShown);
        if (pane->m_headerWidget) {
            pane->m_headerWidget->setActive(paneShown);
        }
    }
=======
    if (m_panel) {
        m_panel->updateHeaderTitle();
        if (m_panel->m_headerWidget) {
            m_panel->m_headerWidget->setActive(rootShown);
        }
    }

    // 副窗格激活判定
    for (ContentPanel* pane : m_panes) {
        if (!pane) continue;
        bool paneActive = (m_activePaneForSplit == pane);
        bool paneShown = isSplit && paneActive;

        pane->updateHeaderTitle();
        setPaneActiveProperty(pane, paneShown);
        if (pane->m_headerWidget) {
            pane->m_headerWidget->setActive(paneShown);
        }
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
Since Qt6 dependencies are not available in the headless sandbox, compilation is skipped.
Functional verification checklist for native build:
1. Launch QuarkMeta in single-pane mode. Confirm top header title is "内容".
2. Split view into 2 or 4 panes (`Ctrl+Shift+L` or split menu). Confirm each pane's header title displays its respective folder name (e.g. "测试", "QuarkMeta_backups", "此电脑").
3. Navigate to another folder inside a split pane. Confirm that pane's header title updates immediately to the new folder name.
4. Close all secondary split panes to return to single-pane mode. Confirm the root pane's header title reverts back to "内容".

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Header Title Updating Channel**: Exposes `ContentHeaderWidget::setTitle` as the single point of truth for setting header label text.
- **Path Display Formatting**: Standardizes clean path formatting across `TabBarWidget` and `ContentPanel` using `QDir::cleanPath` and `QFileInfo::fileName()`.
- **Zero Redundancy**: Reuses existing `isSplitMode()` and `layoutChanged` notifications in `ContentPaneSplitManager` to trigger synchronization without adding redundant timers or polling.

---

## 6. Header API Signature Verification
- `ContentHeaderWidget::setTitle(const QString& title)` -> Added in `ContentHeaderWidget.h` / `ContentHeaderWidget.cpp`.
- `ContentPanel::updateHeaderTitle()` -> Added as private helper in `ContentPanel.h` / `ContentPanel.cpp`.
- `ContentPanel::isSplitMode() const` -> Existing member function in `ContentPanel.h`.
- `ContentPanel::currentPath() const` / `m_currentPath` -> Existing member variable in `ContentPanel.h`.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `#include <QDir>` added to `ContentPanel.cpp` for `QDir::cleanPath`.
- `#include <QFileInfo>` is already present in `ContentPanel.cpp` and `ContentHeaderWidget.cpp`.
- All types (`ContentHeaderWidget`, `ContentPanel`, `ContentPaneSplitManager`, `QString`, `QFileInfo`, `QDir`) have complete header definitions included.
