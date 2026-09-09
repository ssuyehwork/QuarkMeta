# PanelLayoutManager Dynamic Minimum Size Decoupling Implementation Plan

## 1. Overview
In `PanelLayoutManager::updateDynamicMinimumSize()`, `m_mainWindow->setMinimumWidth(finalMinW)` dynamically forced `MainWindow`'s minimum width up to `1180px` when all 5 panels were visible. During window maximization, Win32 `WM_GETMINMAXINFO` passed `1180px` to `mmi->ptMinTrackSize.x`. On smaller screens or during high-rate layout recalculation, this rigid window-level minimum constraint collided with the display work area, forcing multiple layout/repaint cycles and causing black frame flashing during maximization transitions.

This plan decouples `m_mainWindow->setMinimumWidth` in `PanelLayoutManager::updateDynamicMinimumSize()` to strictly use the fixed global floor `kWindowAbsoluteMinWidth` (`475px`), allowing individual `QSplitter` panels (which already have `setMinimumWidth(230)` protection) to manage their minimum panel sizes without causing Native Win32 `WM_GETMINMAXINFO` size conflicts.

## 2. Modified Files List
- `src/ui/PanelLayoutManager.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/PanelLayoutManager.cpp`

```diff
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

    // 🚀【安全解耦】：顶层 MainWindow 维持全局 475px 绝对下限保护，解耦与 WM_GETMINMAXINFO 的最大化尺寸冲突
    m_mainWindow->setMinimumWidth(kWindowAbsoluteMinWidth);
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Compile the application using CMake / MSVC build pipeline.
2. Launch QuarkMeta with all 5 panels expanded.
3. Click the maximize button or double-click the title bar.
4. Verify that window maximization transitions smoothly without layout conflict black frame flashes.
