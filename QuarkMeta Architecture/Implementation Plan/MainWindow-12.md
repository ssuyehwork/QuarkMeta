# MainWindow Implementation Plan

## Overview
This plan updates `src/ui/MainWindow.cpp` to use the exact `显示收藏栏+内容面板+筛选器` SVG icon for `m_btnPresetLayout`'s context menu actions.

## Modified Files List
- `src/ui/MainWindow.cpp`

## Detailed Line-by-Line Changes

### `src/ui/MainWindow.cpp`

```diff
<<<<<<< SEARCH
        QAction* actFav = menu.addAction(UiHelper::getIcon("star_filled", QColor("#EEEEEE")), "显示 收藏栏 + 内容面板 + 筛选器");
        actFav->setCheckable(true);
        actFav->setChecked(currentLeft == "favorite");

        QAction* actNav = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "显示 目录导航 + 内容面板 + 筛选器");
=======
        QAction* actFav = menu.addAction(UiHelper::getIcon("显示收藏栏+内容面板+筛选器", QColor("#EEEEEE")), "显示 收藏栏 + 内容面板 + 筛选器");
        actFav->setCheckable(true);
        actFav->setChecked(currentLeft == "favorite");

        QAction* actNav = menu.addAction(UiHelper::getIcon("显示收藏栏+内容面板+筛选器", QColor("#EEEEEE")), "显示 目录导航 + 内容面板 + 筛选器");
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Inspect `src/ui/MainWindow.cpp` to verify `UiHelper::getIcon("显示收藏栏+内容面板+筛选器", QColor("#EEEEEE"))` is passed to context menu actions.
2. Confirm menu items render the exact SVG icon matching the preset layout button.
