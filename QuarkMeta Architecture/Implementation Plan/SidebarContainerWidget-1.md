# Implementation Plan - SidebarContainerWidget & Style QSS TabBar Base Line Removal

## Overview
Fix redundant horizontal lines appearing below the `SidebarTabBar` (`收藏夹` / `库`) in `SidebarContainerWidget`. The issue is caused by Qt's native `QTabBar` drawing its default base line (`drawBase = true`) overlapping with `#ContainerHeader`'s `border-bottom: 1px solid #333333;`. By explicitly disabling `m_tabBar->setDrawBase(false)` and setting `qproperty-drawBase: false` in `style.qss`, the redundant tab base line is eliminated.

## Modified Files List
- `src/ui/SidebarContainerWidget.cpp`
- `resources/style.qss`

## Detailed Line-by-Line Changes

### 1. `src/ui/SidebarContainerWidget.cpp`
Disable `drawBase` on `m_tabBar`:

```
<<<<<<< SEARCH
    m_tabBar = new QTabBar(header);
    m_tabBar->setObjectName("SidebarTabBar");
    m_tabBar->addTab("收藏夹");
=======
    m_tabBar = new QTabBar(header);
    m_tabBar->setObjectName("SidebarTabBar");
    m_tabBar->setDrawBase(false);
    m_tabBar->addTab("收藏夹");
>>>>>>> REPLACE
```

### 2. `resources/style.qss`
Add `qproperty-drawBase: false` to `QTabBar#SidebarTabBar`:

```
<<<<<<< SEARCH
QTabBar#SidebarTabBar {
    background: transparent;
    border: none;
}
=======
QTabBar#SidebarTabBar {
    background: transparent;
    border: none;
    qproperty-drawBase: false;
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Verify `m_tabBar->setDrawBase(false)` is invoked in `SidebarContainerWidget::initUi`.
2. Verify `qproperty-drawBase: false` is present in `resources/style.qss`.

## SSOT API Reuse & Anti-Redundancy Self-Check
- Modifies existing `QTabBar` configuration directly in `SidebarContainerWidget.cpp` and `resources/style.qss`.
- No duplicate implementations created.

## Header API Signature Verification
- `QTabBar::setDrawBase(bool drawBase)` is a standard Qt GUI method on `QTabBar`.

## Header Inclusion Chain & Type Completeness Check
- `SidebarContainerWidget.h` includes `<QTabBar>`, ensuring complete class definition.
