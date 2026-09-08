# MainWindow Implementation Plan

## Overview
This plan adjusts the layout order of status bar control buttons on the bottom-right of `MainWindow` (`src/ui/MainWindow.cpp`). Per the user's explicit requirement, the button placement order in `setupStatusBar` is inverted to place reset and preset layout actions on the left, followed by navigation, favorite, content, metadata, and filter toggles.

## Modified Files List
- `src/ui/MainWindow.cpp`

## Detailed Line-by-Line Changes

### `src/ui/MainWindow.cpp`

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

## Build & Verification Steps
1. Verify `src/ui/MainWindow.cpp` layout code update.
2. Ensure status bar control buttons display in the updated sequence from left to right: Reset Layout, Preset Layout, Toggle Navigation, Toggle Favorites, Content Panel (Immersive), Toggle Metadata, Toggle Filter.
