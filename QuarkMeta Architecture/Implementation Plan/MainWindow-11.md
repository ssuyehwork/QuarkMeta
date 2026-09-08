# MainWindow Implementation Plan

## Overview
This plan updates the preset layout context menu icons in `src/ui/MainWindow.cpp` to align with the system's 100% semantic SVG icon policy:
1. **Semantic Menu Icons**: Update `m_btnPresetLayout`'s context menu actions to use `star_filled` for the "Favorite + Content + Filter" preset action, and `folder_filled` for the "Nav + Content + Filter" preset action.
2. **Status Bar Layout Order**: Keep buttons ordered as: Reset (far left), Preset, Nav Toggle, Favorite Toggle, Content Panel, Meta Toggle, Filter Toggle (far right).

## Modified Files List
- `src/ui/MainWindow.cpp`

## Detailed Line-by-Line Changes

### `src/ui/MainWindow.cpp`

```diff
<<<<<<< SEARCH
        QAction* actFav = menu.addAction(UiHelper::getIcon("bookmark", QColor("#EEEEEE")), "显示 收藏栏 + 内容面板 + 筛选器");
        actFav->setCheckable(true);
        actFav->setChecked(currentLeft == "favorite");

        QAction* actNav = menu.addAction(UiHelper::getIcon("sidebar", QColor("#EEEEEE")), "显示 目录导航 + 内容面板 + 筛选器");
=======
        QAction* actFav = menu.addAction(UiHelper::getIcon("star_filled", QColor("#EEEEEE")), "显示 收藏栏 + 内容面板 + 筛选器");
        actFav->setCheckable(true);
        actFav->setChecked(currentLeft == "favorite");

        QAction* actNav = menu.addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "显示 目录导航 + 内容面板 + 筛选器");
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `m_btnPresetLayout` right-click context menu items render `star_filled` for Favorites preset and `folder_filled` for Nav preset.
2. Confirm icons render in neutral monochrome `#EEEEEE` with 10px spacing.
