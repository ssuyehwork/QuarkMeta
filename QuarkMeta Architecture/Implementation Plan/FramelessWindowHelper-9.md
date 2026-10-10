# Implementation Plan - FramelessWindowHelper WindowRole Refactoring & Dialog Integration

## 1. Overview
This implementation plan refactors `FramelessWindowHelper` to introduce explicit window roles (`WindowRole::Primary`, `WindowRole::Tool`, `WindowRole::Dialog`) and capability configuration mapping (`RoleCapabilities`).

Key objectives:
1. Replace scattered implicit `if (m_titleBar)` checks with explicit role capability evaluations (`nativeChrome`, `titleBarDrag`, `doubleClickMaximize`, `syncMaximizeIcon`).
2. Unify all frameless windows (`MainWindow`, `TagSelectorOverlay`, and `FramelessDialog`) under `FramelessWindowHelper`'s native Win32 `WM_NCHITTEST` handling.
3. Eliminate duplicate manual mouse dragging (`ReleaseCapture() + SendMessageW(WM_NCLBUTTONDOWN, HTCAPTION, 0)`) in `FramelessDialog`.

---

## 2. Modified Files List
- `src/ui/FramelessWindowHelper.h`
- `src/ui/FramelessWindowHelper.cpp`
- `src/ui/MainWindow.cpp`
- `src/ui/TagSelectorOverlay.cpp`
- `src/ui/FramelessDialogBase.h`
- `src/ui/FramelessDialog.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/FramelessWindowHelper.h`
Add `WindowRole` enum class, update `apply` and constructor signatures, and add `m_role` member variable:

```cpp
<<<<<<< SEARCH
namespace QuarkMeta {

/**
 * @brief 工业级无边框窗口助手类
 * 完全基于 Windows 原生 WM_NCHITTEST 进行硬件级缩放与拖拽，不使用暴力 grabMouse
 */
class FramelessWindowHelper : public QObject {
    Q_OBJECT

public:
    static FramelessWindowHelper* apply(QWidget* window, QWidget* titleBar = nullptr);
    static void setAlwaysOnTop(QWidget* window, bool onTop);
    static bool isAlwaysOnTop(QWidget* window);

    bool handleNativeEvent(void* message, qintptr* result);
    static bool isInteractiveWidget(QWidget* child, QWidget* titleBar, QWidget* window);

private:
    explicit FramelessWindowHelper(QWidget* window, QWidget* titleBar = nullptr);
    ~FramelessWindowHelper() override = default;

    QPointer<QWidget> m_window;
    QPointer<QWidget> m_titleBar;

    static constexpr int kBaseResizeMargin = 8;
};
=======
namespace QuarkMeta {

/**
 * @brief 无边框窗口的角色类型——决定该窗口应具备哪些原生能力
 */
enum class WindowRole {
    Primary,   // 主窗口：完整标题栏拖拽、最大化/还原、系统菜单、边缘缩放
    Tool,      // 悬浮工具窗：无标题栏、不可最大化，仅边缘缩放
    Dialog     // 对话框：有自定义标题栏、可拖拽/最大化，但不需要标题栏最大化图标原生同步
};

/**
 * @brief 工业级无边框窗口助手类
 * 完全基于 Windows 原生 WM_NCHITTEST 进行硬件级缩放与拖拽，不使用暴力 grabMouse
 */
class FramelessWindowHelper : public QObject {
    Q_OBJECT

public:
    static FramelessWindowHelper* apply(QWidget* window, WindowRole role, QWidget* titleBar = nullptr);
    static void setAlwaysOnTop(QWidget* window, bool onTop);
    static bool isAlwaysOnTop(QWidget* window);

    bool handleNativeEvent(void* message, qintptr* result);
    static bool isInteractiveWidget(QWidget* child, QWidget* titleBar, QWidget* window);

private:
    explicit FramelessWindowHelper(QWidget* window, WindowRole role, QWidget* titleBar);
    ~FramelessWindowHelper() override = default;

    QPointer<QWidget> m_window;
    QPointer<QWidget> m_titleBar;
    WindowRole m_role;

    static constexpr int kBaseResizeMargin = 8;
};
>>>>>>> REPLACE
```

### 3.2 `src/ui/FramelessWindowHelper.cpp`
Introduce `RoleCapabilities` struct and `capabilitiesFor`, update `apply` and constructor to store `m_role` and evaluate capabilities, declare `caps` at top of `handleNativeEvent`, and gate role-dependent behavior:

```cpp
<<<<<<< SEARCH
namespace QuarkMeta {

FramelessWindowHelper* FramelessWindowHelper::apply(QWidget* window, QWidget* titleBar) {
    if (!window) return nullptr;
    return new FramelessWindowHelper(window, titleBar);
}

FramelessWindowHelper::FramelessWindowHelper(QWidget* window, QWidget* titleBar)
    : QObject(window), m_window(window), m_titleBar(titleBar) {

    Qt::WindowFlags requiredFlags = m_window->windowFlags() | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint;
    if (m_window->windowFlags() != requiredFlags) {
        m_window->setWindowFlags(requiredFlags);
    }

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);
    // 只有真正带标题栏（传入了 titleBar）的窗口，才需要完整的系统窗口属性
    // 像 TagSelectorOverlay 这种 Qt::Tool 悬浮面板，不该被强行赋予标题栏/最大化/系统菜单语义
    if (m_titleBar) {
        SetWindowLong(hwnd, GWL_STYLE, style | WS_THICKFRAME | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU);
    } else {
        SetWindowLong(hwnd, GWL_STYLE, style | WS_THICKFRAME);
    }
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
#endif
}
=======
namespace QuarkMeta {

namespace {
struct RoleCapabilities {
    bool nativeChrome;        // 是否注入 WS_CAPTION|WS_MAXIMIZEBOX|WS_MINIMIZEBOX|WS_SYSMENU
    bool titleBarDrag;        // 标题栏区域是否响应原生 HTCAPTION 拖拽
    bool doubleClickMaximize; // 双击标题栏是否触发原生最大化/还原
    bool syncMaximizeIcon;    // WM_SIZE/WM_WINDOWPOSCHANGED 时是否回调标题栏同步最大化图标
};

RoleCapabilities capabilitiesFor(WindowRole role) {
    switch (role) {
        case WindowRole::Primary:
            return { true, true, true, true };
        case WindowRole::Tool:
            return { false, false, false, false };
        case WindowRole::Dialog:
            return { true, true, true, false };
    }
    return { false, false, false, false };
}
} // namespace

FramelessWindowHelper* FramelessWindowHelper::apply(QWidget* window, WindowRole role, QWidget* titleBar) {
    if (!window) return nullptr;
    return new FramelessWindowHelper(window, role, titleBar);
}

FramelessWindowHelper::FramelessWindowHelper(QWidget* window, WindowRole role, QWidget* titleBar)
    : QObject(window), m_window(window), m_titleBar(titleBar), m_role(role) {

    Qt::WindowFlags requiredFlags = m_window->windowFlags() | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint;
    if (m_window->windowFlags() != requiredFlags) {
        m_window->setWindowFlags(requiredFlags);
    }

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(m_window->winId());
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);

    const RoleCapabilities caps = capabilitiesFor(m_role);
    if (caps.nativeChrome) {
        SetWindowLong(hwnd, GWL_STYLE, style | WS_THICKFRAME | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU);
    } else {
        SetWindowLong(hwnd, GWL_STYLE, style | WS_THICKFRAME);
    }
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
#endif
}
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    HWND hwnd = msg->hwnd;
    // 关键修正 2：必须以 Win32 原生权威状态为唯一准绳，严禁使用状态滞后的 m_window->isMaximized()
    const bool isMax = ::IsZoomed(hwnd);

    // 0. 尺寸与位置变动原生分发：第一时间校准标题栏最大化/还原图标
    if (msg->message == WM_SIZE || msg->message == WM_WINDOWPOSCHANGED) {
        if (m_titleBar) {
            QMetaObject::invokeMethod(m_titleBar, "setWindowMaximized", Q_ARG(bool, isMax));
        }
    }
=======
    HWND hwnd = msg->hwnd;
    // 关键修正 2：必须以 Win32 原生权威状态为唯一准绳，严禁使用状态滞后的 m_window->isMaximized()
    const bool isMax = ::IsZoomed(hwnd);
    const RoleCapabilities caps = capabilitiesFor(m_role);

    // 0. 尺寸与位置变动原生分发：第一时间校准标题栏最大化/还原图标
    if (msg->message == WM_SIZE || msg->message == WM_WINDOWPOSCHANGED) {
        if (caps.syncMaximizeIcon && m_titleBar) {
            QMetaObject::invokeMethod(m_titleBar, "setWindowMaximized", Q_ARG(bool, isMax));
        }
    }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
        // 标题栏原生拖拽与双击识别（排除交互控件）
        if (m_titleBar && !m_window->isFullScreen()) {
            QRect titleRect = QRect(m_titleBar->mapTo(m_window, QPoint(0, 0)), m_titleBar->size());
            if (titleRect.contains(localPos)) {
                QWidget* childAtPt = m_window->childAt(localPos);
                if (!isInteractiveWidget(childAtPt, m_titleBar, m_window)) {
                    *result = HTCAPTION;
                    return true;
                }
            }
        }
=======
        // 标题栏原生拖拽与双击识别（排除交互控件）
        if (caps.titleBarDrag && m_titleBar && !m_window->isFullScreen()) {
            QRect titleRect = QRect(m_titleBar->mapTo(m_window, QPoint(0, 0)), m_titleBar->size());
            if (titleRect.contains(localPos)) {
                QWidget* childAtPt = m_window->childAt(localPos);
                if (!isInteractiveWidget(childAtPt, m_titleBar, m_window)) {
                    *result = HTCAPTION;
                    return true;
                }
            }
        }
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    // 5. 原生双击标题栏最大化 / 还原（通过 Win32 消息总线响应）
    if (msg->message == WM_NCLBUTTONDBLCLK) {
        if (msg->wParam == HTCAPTION) {
            ::SendMessage(hwnd, WM_SYSCOMMAND, isMax ? SC_RESTORE : SC_MAXIMIZE, 0);
            *result = 0;
            return true;
        }
    }
=======
    // 5. 原生双击标题栏最大化 / 还原（通过 Win32 消息总线响应）
    if (msg->message == WM_NCLBUTTONDBLCLK) {
        if (caps.doubleClickMaximize && msg->wParam == HTCAPTION) {
            ::SendMessage(hwnd, WM_SYSCOMMAND, isMax ? SC_RESTORE : SC_MAXIMIZE, 0);
            *result = 0;
            return true;
        }
    }
>>>>>>> REPLACE
```

### 3.3 `src/ui/MainWindow.cpp`
Update `FramelessWindowHelper::apply` call to pass `WindowRole::Primary`:

```cpp
<<<<<<< SEARCH
    // 挂载无边框助手（必须在几何属性 restoreGeometry 恢复前完成挂载）
    m_framelessHelper = FramelessWindowHelper::apply(this, m_titleBarWidget);
=======
    // 挂载无边框助手（必须在几何属性 restoreGeometry 恢复前完成挂载）
    m_framelessHelper = FramelessWindowHelper::apply(this, WindowRole::Primary, m_titleBarWidget);
>>>>>>> REPLACE
```

### 3.4 `src/ui/TagSelectorOverlay.cpp`
Update `FramelessWindowHelper::apply` call to pass `WindowRole::Tool`:

```cpp
<<<<<<< SEARCH
    m_framelessHelper = FramelessWindowHelper::apply(this, nullptr);
=======
    m_framelessHelper = FramelessWindowHelper::apply(this, WindowRole::Tool);
>>>>>>> REPLACE
```

### 3.5 `src/ui/FramelessDialogBase.h`
Forward declare `FramelessWindowHelper`, add member pointers `m_titleBar` and `m_framelessHelper`, and declare `nativeEvent`:

```cpp
<<<<<<< SEARCH
namespace QuarkMeta {

class FramelessDialog : public QDialog {
    Q_OBJECT
public:
=======
namespace QuarkMeta {

class FramelessWindowHelper;

class FramelessDialog : public QDialog {
    Q_OBJECT
public:
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

    QWidget* m_contentArea;
=======
protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

    QWidget* m_titleBar = nullptr;
    FramelessWindowHelper* m_framelessHelper = nullptr;
    QWidget* m_contentArea;
>>>>>>> REPLACE
```

### 3.6 `src/ui/FramelessDialog.cpp`
Include `FramelessWindowHelper.h`, initialize `m_titleBar` and `m_framelessHelper`, delegate `nativeEvent`, and remove manual dragging:

```cpp
<<<<<<< SEARCH
#include "FramelessDialog.h"
#include "UiHelper.h"
=======
#include "FramelessDialog.h"
#include "FramelessWindowHelper.h"
#include "UiHelper.h"
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
    m_mainLayout->addWidget(titleBar);
    m_mainLayout->addSpacing(4);
=======
    m_titleBar = titleBar;
    m_framelessHelper = FramelessWindowHelper::apply(this, WindowRole::Dialog, m_titleBar);

    m_mainLayout->addWidget(titleBar);
    m_mainLayout->addSpacing(4);
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
void FramelessDialog::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QWidget* child = childAt(event->pos());
        if (!child || !isInteractiveWidget(child)) {
#ifdef Q_OS_WIN
            ReleaseCapture();
            ::SendMessageW(reinterpret_cast<HWND>(winId()), WM_NCLBUTTONDOWN, HTCAPTION, 0);
            event->accept();
            return;
#else
            m_isDragging = true;
            m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
            return;
#endif
        }
    }
    QDialog::mousePressEvent(event);
}
=======
void FramelessDialog::mousePressEvent(QMouseEvent* event) {
#ifndef Q_OS_WIN
    if (event->button() == Qt::LeftButton) {
        QWidget* child = childAt(event->pos());
        if (!child || !isInteractiveWidget(child)) {
            m_isDragging = true;
            m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
            return;
        }
    }
#endif
    QDialog::mousePressEvent(event);
}
>>>>>>> REPLACE
```

```cpp
<<<<<<< SEARCH
bool FramelessDialog::eventFilter(QObject* watched, QEvent* event) {
    return QDialog::eventFilter(watched, event);
}

} // namespace QuarkMeta
=======
bool FramelessDialog::eventFilter(QObject* watched, QEvent* event) {
    return QDialog::eventFilter(watched, event);
}

bool FramelessDialog::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    if (m_framelessHelper && m_framelessHelper->handleNativeEvent(message, result)) {
        return true;
    }
    return QDialog::nativeEvent(eventType, message, result);
}

} // namespace QuarkMeta
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Perform CMake configuration and build.
2. Launch QuarkMeta and test window behaviors for `MainWindow` (Primary role: resize edges, title bar drag, double-click maximize/restore, maximize icon sync).
3. Open `TagSelectorOverlay` (Tool role: resize edges, no title bar drag, no maximize).
4. Open any `FramelessDialog` dialog (Dialog role: title bar drag, window resize, no title bar icon sync).

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- `FramelessWindowHelper::apply` serves as the unified SSOT entry point for frameless window initialization.
- Removed duplicate Win32 dragging code from `FramelessDialog.cpp`.

---

## 6. Header API Signature Verification
- `FramelessWindowHelper::apply(QWidget* window, WindowRole role, QWidget* titleBar = nullptr)`
- `FramelessWindowHelper::handleNativeEvent(void* message, qintptr* result)`
- `FramelessDialog::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `FramelessWindowHelper.h` defines `enum class WindowRole`.
- `FramelessDialogBase.h` forward-declares `class FramelessWindowHelper;`.
- `FramelessDialog.cpp` includes `#include "FramelessWindowHelper.h"`.
