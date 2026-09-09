# PanelLayoutManager Dynamic Minimum Width Safeguard Refactoring Implementation Plan

## Overview
This implementation plan corrects the window minimum width protection in `PanelLayoutManager` to prevent UI overlap and panel distortion ("squeezed UI bug" as demonstrated in `image.png`).

1. **Dynamic Minimum Size Safeguard (`updateDynamicMinimumSize`)**:
   - Re-enforces the dynamic minimum width calculation based on the exact count of currently visible panels:
     `calculatedMinW = (visibleCount * kBasePanelWidth) + ((visibleCount - 1) * kSplitterHandleWidth) + 10`
   - For 5 visible panels: enforces `1180px` as the strict lower bound for `m_mainWindow->setMinimumWidth(1180)`, preventing `QSplitter` from crushing visible panels below their usable widths.
   - For 3 visible panels: enforces `710px`.
   - For 1-2 visible panels (or immersive mode): uses `kWindowAbsoluteMinWidth` (`475px`) to protect top bar controls.

2. **Startup Panel State Restoration & Correct Splitting Sequence**:
   - In `initLayout()`, restores individual panel visibility states (`NavVisible`, `FavoriteVisible`, `MetaVisible`, `FilterVisible`) from `AppConfig` when not in immersive mode BEFORE restoring `SplitterState`, ensuring `QSplitter` correctly aligns panel sizes with active visibilities.

3. **Atomic Batch Panel Visibility Updates**:
   - Adds `setBatchPanelVisibility(const QMap<QString, bool>& visibilities)` to `PanelLayoutManager`.
   - Refactors preset layout buttons and batch layout switches in `MainWindow.cpp` to call `setBatchPanelVisibility()` atomically, eliminating multi-pass splitter recalculations, disk write storms (`AppConfig::sync()`), and UI flickering.

## Modified Files List
- `src/ui/PanelLayoutManager.h`
- `src/ui/PanelLayoutManager.cpp`
- `src/ui/MainWindow.cpp`

## Detailed Line-by-Line Changes

### 1. `src/ui/PanelLayoutManager.h`
Add `setBatchPanelVisibility` public method declaration.

<<<<<<< SEARCH
    void initLayout();
    void resetSplitterLayout();
    void setPanelVisible(const QString& panelId, bool visible);
    bool isPanelVisible(const QString& panelId) const;
=======
    void initLayout();
    void resetSplitterLayout();
    void setPanelVisible(const QString& panelId, bool visible);
    void setBatchPanelVisibility(const QMap<QString, bool>& visibilities);
    bool isPanelVisible(const QString& panelId) const;
>>>>>>> REPLACE

---

### 2. `src/ui/PanelLayoutManager.cpp`
Update `initLayout()`, implement `setBatchPanelVisibility()`, and restore strict dynamic minimum size calculation in `updateDynamicMinimumSize()`.

<<<<<<< SEARCH
void PanelLayoutManager::initLayout() {
    if (!m_mainSplitter) return;

    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 0);
    m_mainSplitter->setStretchFactor(2, 1);
    m_mainSplitter->setStretchFactor(3, 0);
    m_mainSplitter->setStretchFactor(4, 0);

    bool isImmersive = AppConfig::instance().getValue("MainWindow/IsImmersiveMode", false).toBool();
    if (isImmersive) {
        if (m_navPanel) m_navPanel->setVisible(false);
        if (m_favoritePanel) m_favoritePanel->setVisible(false);
        if (m_metaPanel) m_metaPanel->setVisible(false);
        if (m_filterPanel) m_filterPanel->setVisible(false);
        emit panelVisibilityChanged("nav", false);
        emit panelVisibilityChanged("favorite", false);
        emit panelVisibilityChanged("meta", false);
        emit panelVisibilityChanged("filter", false);
    }

    // 同步恢复分栏尺寸，杜绝异步 singleShot(0) 造成的二次排版抽搐
    QByteArray state = AppConfig::instance().getValue("MainWindow/SplitterState").toByteArray();
    if (!state.isEmpty() && !isImmersive) {
        m_mainSplitter->restoreState(state);
    } else if (!isImmersive) {
        QList<int> sizes;
        sizes << kBasePanelWidth << kBasePanelWidth << kContentBaseWidth << kBasePanelWidth << kBasePanelWidth;
        m_mainSplitter->setSizes(sizes);
    }
    m_mainSplitter->setHandleWidth(kSplitterHandleWidth);

    // 🚀 初始化完成当场强制焊死最小宽度保护，防止 5 栏被挤压崩溃
    updateDynamicMinimumSize();
}
=======
void PanelLayoutManager::initLayout() {
    if (!m_mainSplitter) return;

    m_mainSplitter->setStretchFactor(0, 0);
    m_mainSplitter->setStretchFactor(1, 0);
    m_mainSplitter->setStretchFactor(2, 1);
    m_mainSplitter->setStretchFactor(3, 0);
    m_mainSplitter->setStretchFactor(4, 0);

    bool isImmersive = AppConfig::instance().getValue("MainWindow/IsImmersiveMode", false).toBool();
    if (isImmersive) {
        if (m_navPanel) m_navPanel->setVisible(false);
        if (m_favoritePanel) m_favoritePanel->setVisible(false);
        if (m_metaPanel) m_metaPanel->setVisible(false);
        if (m_filterPanel) m_filterPanel->setVisible(false);
        emit panelVisibilityChanged("nav", false);
        emit panelVisibilityChanged("favorite", false);
        emit panelVisibilityChanged("meta", false);
        emit panelVisibilityChanged("filter", false);
    } else {
        bool navVis = AppConfig::instance().getValue("MainWindow/NavVisible", true).toBool();
        bool favVis = AppConfig::instance().getValue("MainWindow/FavoriteVisible", true).toBool();
        bool metaVis = AppConfig::instance().getValue("MainWindow/MetaVisible", true).toBool();
        bool filterVis = AppConfig::instance().getValue("MainWindow/FilterVisible", true).toBool();

        if (m_navPanel) m_navPanel->setVisible(navVis);
        if (m_favoritePanel) m_favoritePanel->setVisible(favVis);
        if (m_metaPanel) m_metaPanel->setVisible(metaVis);
        if (m_filterPanel) m_filterPanel->setVisible(filterVis);

        emit panelVisibilityChanged("nav", navVis);
        emit panelVisibilityChanged("favorite", favVis);
        emit panelVisibilityChanged("meta", metaVis);
        emit panelVisibilityChanged("filter", filterVis);
    }

    // 同步恢复分栏尺寸，杜绝异步 singleShot(0) 造成的二次排版抽搐
    QByteArray state = AppConfig::instance().getValue("MainWindow/SplitterState").toByteArray();
    if (!state.isEmpty() && !isImmersive) {
        m_mainSplitter->restoreState(state);
    } else if (!isImmersive) {
        QList<int> sizes;
        sizes << kBasePanelWidth << kBasePanelWidth << kContentBaseWidth << kBasePanelWidth << kBasePanelWidth;
        m_mainSplitter->setSizes(sizes);
    }
    m_mainSplitter->setHandleWidth(kSplitterHandleWidth);

    updateDynamicMinimumSize();
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void PanelLayoutManager::setPanelVisible(const QString& panelId, bool visible) {
    if (panelId == "nav" && m_navPanel) m_navPanel->setVisible(visible);
    else if (panelId == "favorite" && m_favoritePanel) m_favoritePanel->setVisible(visible);
    else if (panelId == "content" && m_contentPanel) m_contentPanel->setVisible(true);
    else if (panelId == "meta" && m_metaPanel) m_metaPanel->setVisible(visible);
    else if (panelId == "filter" && m_filterPanel) m_filterPanel->setVisible(visible);

    updateDynamicMinimumSize();
    saveLayoutState();
    emit panelVisibilityChanged(panelId, visible);
}
=======
void PanelLayoutManager::setPanelVisible(const QString& panelId, bool visible) {
    if (panelId == "nav" && m_navPanel) m_navPanel->setVisible(visible);
    else if (panelId == "favorite" && m_favoritePanel) m_favoritePanel->setVisible(visible);
    else if (panelId == "content" && m_contentPanel) m_contentPanel->setVisible(true);
    else if (panelId == "meta" && m_metaPanel) m_metaPanel->setVisible(visible);
    else if (panelId == "filter" && m_filterPanel) m_filterPanel->setVisible(visible);

    updateDynamicMinimumSize();
    saveLayoutState();
    emit panelVisibilityChanged(panelId, visible);
}

void PanelLayoutManager::setBatchPanelVisibility(const QMap<QString, bool>& visibilities) {
    for (auto it = visibilities.constBegin(); it != visibilities.constEnd(); ++it) {
        const QString& panelId = it.key();
        bool visible = it.value();
        if (panelId == "nav" && m_navPanel) m_navPanel->setVisible(visible);
        else if (panelId == "favorite" && m_favoritePanel) m_favoritePanel->setVisible(visible);
        else if (panelId == "content" && m_contentPanel) m_contentPanel->setVisible(true);
        else if (panelId == "meta" && m_metaPanel) m_metaPanel->setVisible(visible);
        else if (panelId == "filter" && m_filterPanel) m_filterPanel->setVisible(visible);
    }

    updateDynamicMinimumSize();
    saveLayoutState();

    for (auto it = visibilities.constBegin(); it != visibilities.constEnd(); ++it) {
        emit panelVisibilityChanged(it.key(), it.value());
    }
}
>>>>>>> REPLACE

<<<<<<< SEARCH
void PanelLayoutManager::updateDynamicMinimumSize() {
    if (!m_mainWindow) return;

    int visibleCount = 0;
    if (m_navPanel && !m_navPanel->isHidden()) visibleCount++;
    if (m_favoritePanel && !m_favoritePanel->isHidden()) visibleCount++;
    if (m_contentPanel && !m_contentPanel->isHidden()) visibleCount++;
    if (m_metaPanel && !m_metaPanel->isHidden()) visibleCount++;
    if (m_filterPanel && !m_filterPanel->isHidden()) visibleCount++;

    if (visibleCount <= 0) visibleCount = 1;

    // 🚀【绝对不可动摇的刚性物理生命线】：5栏全开时强制锁定 1180px，坚决杜绝再次被挤成肉饼！
    int calculatedMinW = (visibleCount * kBasePanelWidth) + ((visibleCount - 1) * kSplitterHandleWidth) + 10;
    int finalMinW = std::max(kWindowAbsoluteMinWidth, calculatedMinW);

    m_mainWindow->setMinimumWidth(finalMinW);
}
=======
void PanelLayoutManager::updateDynamicMinimumSize() {
    if (!m_mainWindow) return;

    int visibleCount = 0;
    if (m_navPanel && !m_navPanel->isHidden()) visibleCount++;
    if (m_favoritePanel && !m_favoritePanel->isHidden()) visibleCount++;
    if (m_contentPanel && !m_contentPanel->isHidden()) visibleCount++;
    if (m_metaPanel && !m_metaPanel->isHidden()) visibleCount++;
    if (m_filterPanel && !m_filterPanel->isHidden()) visibleCount++;

    if (visibleCount <= 0) visibleCount = 1;

    // 🚀【绝对不可动摇的刚性物理生命线】：5栏全开时强制锁定 1180px，坚决杜绝面板被挤压错乱！
    int calculatedMinW = (visibleCount * kBasePanelWidth) + ((visibleCount - 1) * kSplitterHandleWidth) + 10;
    int finalMinW = std::max(kWindowAbsoluteMinWidth, calculatedMinW);

    m_mainWindow->setMinimumWidth(finalMinW);
}
>>>>>>> REPLACE

---

### 3. `src/ui/MainWindow.cpp`
Use `setBatchPanelVisibility` for status bar panel layout buttons to prevent cascading updates.

<<<<<<< SEARCH
    connect(m_btnToggleFilter, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setPanelVisible("nav", true);
        m_panelLayoutManager->setPanelVisible("favorite", true);
        m_panelLayoutManager->setPanelVisible("content", true);
        m_panelLayoutManager->setPanelVisible("meta", true);
        m_panelLayoutManager->setPanelVisible("filter", false);
        updateStatusBarButtonHighlights();
    });

    connect(m_btnToggleMeta, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setPanelVisible("nav", true);
        m_panelLayoutManager->setPanelVisible("favorite", true);
        m_panelLayoutManager->setPanelVisible("content", true);
        m_panelLayoutManager->setPanelVisible("meta", false);
        m_panelLayoutManager->setPanelVisible("filter", true);
        updateStatusBarButtonHighlights();
    });

    connect(m_btnContentPanel, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setPanelVisible("nav", false);
        m_panelLayoutManager->setPanelVisible("favorite", false);
        m_panelLayoutManager->setPanelVisible("content", true);
        m_panelLayoutManager->setPanelVisible("meta", false);
        m_panelLayoutManager->setPanelVisible("filter", false);
        updateStatusBarButtonHighlights();
    });

    connect(m_btnToggleFavorite, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setPanelVisible("nav", true);
        m_panelLayoutManager->setPanelVisible("favorite", false);
        m_panelLayoutManager->setPanelVisible("content", true);
        m_panelLayoutManager->setPanelVisible("meta", true);
        m_panelLayoutManager->setPanelVisible("filter", true);
        updateStatusBarButtonHighlights();
    });

    connect(m_btnToggleNav, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setPanelVisible("nav", false);
        m_panelLayoutManager->setPanelVisible("favorite", true);
        m_panelLayoutManager->setPanelVisible("content", true);
        m_panelLayoutManager->setPanelVisible("meta", true);
        m_panelLayoutManager->setPanelVisible("filter", true);
        updateStatusBarButtonHighlights();
    });
=======
    connect(m_btnToggleFilter, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setBatchPanelVisibility({
            {"nav", true}, {"favorite", true}, {"content", true}, {"meta", true}, {"filter", false}
        });
        updateStatusBarButtonHighlights();
    });

    connect(m_btnToggleMeta, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setBatchPanelVisibility({
            {"nav", true}, {"favorite", true}, {"content", true}, {"meta", false}, {"filter", true}
        });
        updateStatusBarButtonHighlights();
    });

    connect(m_btnContentPanel, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setBatchPanelVisibility({
            {"nav", false}, {"favorite", false}, {"content", true}, {"meta", false}, {"filter", false}
        });
        updateStatusBarButtonHighlights();
    });

    connect(m_btnToggleFavorite, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setBatchPanelVisibility({
            {"nav", true}, {"favorite", false}, {"content", true}, {"meta", true}, {"filter", true}
        });
        updateStatusBarButtonHighlights();
    });

    connect(m_btnToggleNav, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setBatchPanelVisibility({
            {"nav", false}, {"favorite", true}, {"content", true}, {"meta", true}, {"filter", true}
        });
        updateStatusBarButtonHighlights();
    });
>>>>>>> REPLACE

<<<<<<< SEARCH
    connect(m_btnResetLayout, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setPanelVisible("nav", true);
        m_panelLayoutManager->setPanelVisible("favorite", true);
        m_panelLayoutManager->setPanelVisible("content", true);
        m_panelLayoutManager->setPanelVisible("meta", true);
        m_panelLayoutManager->setPanelVisible("filter", true);
        m_panelLayoutManager->resetSplitterLayout();
        updateStatusBarButtonHighlights();
    });
=======
    connect(m_btnResetLayout, &QPushButton::clicked, this, [this]() {
        if (!m_panelLayoutManager) return;
        m_panelLayoutManager->setBatchPanelVisibility({
            {"nav", true}, {"favorite", true}, {"content", true}, {"meta", true}, {"filter", true}
        });
        m_panelLayoutManager->resetSplitterLayout();
        updateStatusBarButtonHighlights();
    });
>>>>>>> REPLACE

<<<<<<< SEARCH
void MainWindow::applyPresetLayout(const QString& leftPanel) {
    if (!m_panelLayoutManager) return;
    if (leftPanel == "nav") {
        m_panelLayoutManager->setPanelVisible("nav", true);
        m_panelLayoutManager->setPanelVisible("favorite", false);
    } else {
        m_panelLayoutManager->setPanelVisible("favorite", true);
        m_panelLayoutManager->setPanelVisible("nav", false);
    }
    m_panelLayoutManager->setPanelVisible("content", true);
    m_panelLayoutManager->setPanelVisible("meta", false);
    m_panelLayoutManager->setPanelVisible("filter", true);
    updateStatusBarButtonHighlights();
}
=======
void MainWindow::applyPresetLayout(const QString& leftPanel) {
    if (!m_panelLayoutManager) return;
    bool showNav = (leftPanel == "nav");
    m_panelLayoutManager->setBatchPanelVisibility({
        {"nav", showNav},
        {"favorite", !showNav},
        {"content", true},
        {"meta", false},
        {"filter", true}
    });
    updateStatusBarButtonHighlights();
}
>>>>>>> REPLACE

## Build & Verification Steps
1. Verify `PanelLayoutManager-5.md` is registered in `QuarkMeta Architecture/Implementation Plan/`.
2. Ensure dynamic minimum width calculation `updateDynamicMinimumSize()` matches `(visibleCount * 230) + ((visibleCount - 1) * 5) + 10`.
