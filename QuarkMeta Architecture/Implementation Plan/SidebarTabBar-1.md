# Implementation Plan - SidebarTabBar Background Highlight Styling

## 1. Overview
This implementation plan updates the QSS styling for `QTabBar#SidebarTabBar` in `resources/style.qss`:
1. Replaces the bottom blue underline indicator styling (`border-bottom: 2px solid #378ADD`) with solid background card highlighting (`background: #37373D` for selected tabs, `background: #2A2D2E` for hover states, `border-radius: 4px`).

---

## 2. Modified Files List
- `resources/style.qss`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `resources/style.qss`
Update `QTabBar#SidebarTabBar` style rules:

```css
<<<<<<< SEARCH
QTabBar#SidebarTabBar {
    background: transparent;
    border: none;
}

QTabBar#SidebarTabBar::tab {
    background: transparent;
    color: #888888;
    padding: 4px 12px;
    font-size: 13px;
    font-weight: bold;
    border: none;
    border-bottom: 2px solid transparent;
}

QTabBar#SidebarTabBar::tab:selected {
    color: #FFFFFF;
    border-bottom: 2px solid #378ADD;
}

QTabBar#SidebarTabBar::tab:hover:!selected {
    color: #CCCCCC;
}
=======
QTabBar#SidebarTabBar {
    background: transparent;
    border: none;
}

QTabBar#SidebarTabBar::tab {
    background: transparent;
    color: #AAAAAA;
    padding: 4px 12px;
    font-size: 13px;
    font-weight: bold;
    border: none;
    border-radius: 4px;
    margin-right: 4px;
}

QTabBar#SidebarTabBar::tab:selected {
    background: #37373D;
    color: #FFFFFF;
}

QTabBar#SidebarTabBar::tab:hover:!selected {
    background: #2A2D2E;
    color: #CCCCCC;
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Launch QuarkMeta application.
2. Observe the "收藏夹 / 库" sidebar tab bar header.
3. Verify that selected tabs display dark rounded card background highlighting (`#37373D`) without bottom blue underlines.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Centralizes QSS styling in `resources/style.qss` in compliance with Rule 4.1.

---

## 6. Header API Signature Verification
- N/A (CSS QSS stylesheet modification only).

---

## 7. Header Inclusion Chain & Type Completeness Check
- N/A (CSS QSS stylesheet modification only).
