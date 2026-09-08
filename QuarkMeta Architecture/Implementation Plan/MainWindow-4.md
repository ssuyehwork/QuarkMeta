# MainWindow Implementation Plan

## Overview
This plan addresses two issues in `src/ui/MainWindow.cpp` regarding the status bar control buttons:
1. **Invert Button Layout Order**: Reverse the layout order of the 7 status bar control buttons on the bottom-right so that reset and preset layout actions appear on the far left, followed by navigation, favorite, content, metadata, and filter toggles.
2. **Fix Duplicate ToolTip Overlay**: Remove `btn->setToolTip(tip);` in `createStatusBtn`. Native Qt tooltips were firing concurrently with `HoverEventFilter`'s `ToolTipOverlay` popup, causing doubled/overlapped tooltip boxes on button hover.

## Modified Files List
- `src/ui/MainWindow.cpp`

## Detailed Line-by-Line Changes

### `src/ui/MainWindow.cpp`

```diff
<<<<<<< SEARCH
        btn->setProperty("tooltipText", tip);
        btn->setToolTip(tip);
        if (m_hoverFilter) {
=======
        btn->setProperty("tooltipText", tip);
        if (m_hoverFilter) {
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
    statusL->setSpacing(4);
    statusL->addWidget(m_btnToggleFilter);
    statusL->addWidget(m_btnToggleMeta);
    statusL->addWidget(m_btnContentPanel);
    statusL->addWidget(m_btnToggleFavorite);
    statusL->addWidget(m_btnToggleNav);
    statusL->addWidget(m_btnPresetLayout);
    statusL->addWidget(m_btnResetLayout);
=======
    statusL->setSpacing(4);
    statusL->addWidget(m_btnResetLayout);
    statusL->addWidget(m_btnPresetLayout);
    statusL->addWidget(m_btnToggleNav);
    statusL->addWidget(m_btnToggleFavorite);
    statusL->addWidget(m_btnContentPanel);
    statusL->addWidget(m_btnToggleMeta);
    statusL->addWidget(m_btnToggleFilter);
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `src/ui/MainWindow.cpp` layout code update.
2. Ensure status bar control buttons display in the updated sequence from left to right: Reset Layout, Preset Layout, Toggle Navigation, Toggle Favorites, Content Panel (Immersive), Toggle Metadata, Toggle Filter.
3. Verify that hovering over any status bar control button displays only a single, clean `ToolTipOverlay` popup without native Qt tooltip box overlapping.
