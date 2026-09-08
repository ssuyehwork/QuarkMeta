# MainWindow Implementation Plan

## Overview
This plan updates the icon size of the status bar control buttons in `src/ui/MainWindow.cpp` to `27 x 24 px` (`btn->setIconSize(QSize(27, 24));`), while keeping all other parameters unchanged.

## Modified Files List
- `src/ui/MainWindow.cpp`

## Detailed Line-by-Line Changes

### `src/ui/MainWindow.cpp`

```diff
<<<<<<< SEARCH
        btn->setIconSize(QSize(26, 20));
=======
        btn->setIconSize(QSize(27, 24));
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Inspect `src/ui/MainWindow.cpp` to verify `btn->setIconSize(QSize(27, 24));` is set.
2. Confirm status bar buttons render with the 27x24 icon size.
