#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "FramelessWindowHelper.h"
#include <QMouseEvent>
#include <QScreen>
#include <QWindow>
#include <QApplication>
#include <QCoreApplication>
#include <QPushButton>
#include <QLineEdit>
#include <QToolButton>
#include <QSlider>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

namespace QuarkMeta {

void FramelessWindowHelper::apply(QWidget* window, QWidget* titleBar) {
    if (!window) return;
    new FramelessWindowHelper(window, titleBar);
}

FramelessWindowHelper::FramelessWindowHelper(QWidget* window, QWidget* titleBar)
    : QObject(window), m_window(window), m_titleBar(titleBar) {
    
    Qt::WindowFlags requiredFlags = m_window->windowFlags() | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint;
    if (m_window->windowFlags() != requiredFlags) {
        m_window->setWindowFlags(requiredFlags);
    }
    
    QCoreApplication::instance()->installEventFilter(this);
    QCoreApplication::instance()->installNativeEventFilter(this);
}

FramelessWindowHelper::~FramelessWindowHelper() {
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->removeNativeEventFilter(this);
    }
}

bool FramelessWindowHelper::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) {
    Q_UNUSED(eventType);
#ifdef Q_OS_WIN
    MSG* msg = static_cast<MSG*>(message);
    if (!m_window || reinterpret_cast<HWND>(m_window->winId()) != msg->hwnd) {
        return false;   // 只处理自己管的这个窗口，不要拦别的窗口的消息
    }

    // 1. 客户区计算：整个窗口都是客户区，不留原生标题栏/边框空间
    if (msg->message == WM_NCCALCSIZE) {
        *result = 0;
        return true;
    }

    // 2. 最大化尺寸修正：避免盖住任务栏
    if (msg->message == WM_GETMINMAXINFO) {
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(msg->lParam);
        HMONITOR monitor = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST);
        if (monitor) {
            MONITORINFO monitorInfo = {};
            monitorInfo.cbSize = sizeof(MONITORINFO);
            GetMonitorInfo(monitor, &monitorInfo);
            RECT workArea = monitorInfo.rcWork;
            RECT monitorArea = monitorInfo.rcMonitor;
            mmi->ptMaxPosition.x = workArea.left - monitorArea.left;
            mmi->ptMaxPosition.y = workArea.top - monitorArea.top;
            mmi->ptMaxSize.x = workArea.right - workArea.left;
            mmi->ptMaxSize.y = workArea.bottom - workArea.top;
        }
        *result = 0;
        return true;
    }

    // 3. 命中测试：告诉Windows鼠标现在停在哪个可交互区域（标题栏/边缘），把移动和缩放完全交给系统原生处理
    if (msg->message == WM_NCHITTEST) {
        if (m_window->isMaximized() || m_window->isFullScreen()) {
            return false;   // 最大化时不需要边缘缩放判定
        }

        const LONG dpi = GetDpiForWindow(msg->hwnd);
        const int margin = MulDiv(kBaseResizeMargin, dpi, 96);

        POINT screenPt = { GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };
        RECT wr;
        GetWindowRect(msg->hwnd, &wr);
        int x = screenPt.x - wr.left;
        int y = screenPt.y - wr.top;
        int w = wr.right - wr.left;
        int h = wr.bottom - wr.top;

        bool left   = x >= 0 && x < margin;
        bool right  = x >= w - margin && x < w;
        bool top    = y >= 0 && y < margin;
        bool bottom = y >= h - margin && y < h;

        if (top && left)     { *result = HTTOPLEFT;     return true; }
        if (top && right)    { *result = HTTOPRIGHT;    return true; }
        if (bottom && left)  { *result = HTBOTTOMLEFT;  return true; }
        if (bottom && right) { *result = HTBOTTOMRIGHT; return true; }
        if (left)            { *result = HTLEFT;        return true; }
        if (right)           { *result = HTRIGHT;       return true; }
        if (top)             { *result = HTTOP;         return true; }
        if (bottom)          { *result = HTBOTTOM;      return true; }

        // 落在标题栏区域内、且不是按钮控件，交给系统原生拖拽移动
        if (m_titleBar) {
            QPoint localPt = m_window->mapFromGlobal(QPoint(screenPt.x, screenPt.y));
            QWidget* childAtPt = m_window->childAt(localPt);
            bool inTitleBar = m_titleBar->rect().contains(m_titleBar->mapFromGlobal(QPoint(screenPt.x, screenPt.y)));

            bool isInteractive = false;
            QWidget* wWidget = childAtPt;
            while (wWidget && wWidget != m_titleBar && wWidget != m_window) {
                if (qobject_cast<QPushButton*>(wWidget) ||
                    qobject_cast<QToolButton*>(wWidget) ||
                    qobject_cast<QLineEdit*>(wWidget) ||
                    qobject_cast<QSlider*>(wWidget)) {
                    isInteractive = true;
                    break;
                }
                wWidget = wWidget->parentWidget();
            }

            if (inTitleBar && !isInteractive) {
                *result = HTCAPTION;
                return true;
            }
        }
    }
#else
    Q_UNUSED(message);
    Q_UNUSED(result);
#endif
    return false;
}

void FramelessWindowHelper::setAlwaysOnTop(QWidget* window, bool onTop) {
    if (!window) return;

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    SetWindowPos(hwnd, onTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
#else
    Qt::WindowFlags flags = window->windowFlags();
    if (onTop) flags |= Qt::WindowStaysOnTopHint;
    else flags &= ~Qt::WindowStaysOnTopHint;
    window->setWindowFlags(flags);
    window->show();
#endif
}

bool FramelessWindowHelper::isAlwaysOnTop(QWidget* window) {
    if (!window) return false;
    return (window->windowFlags() & Qt::WindowStaysOnTopHint) != 0;
}

bool FramelessWindowHelper::eventFilter(QObject* obj, QEvent* event) {
    if (!m_window || !m_window->isVisible()) return false;

    QWidget* widget = qobject_cast<QWidget*>(obj);
    if (!widget || (widget != m_window && !m_window->isAncestorOf(widget))) {
        return false;
    }

    if (event->type() == QEvent::MouseButtonDblClick && m_titleBar &&
        (widget == m_titleBar || m_titleBar->isAncestorOf(widget))) {
        bool isInteractive = false;
        QWidget* wWidget = widget;
        while (wWidget && wWidget != m_titleBar && wWidget != m_window) {
            if (qobject_cast<QPushButton*>(wWidget) ||
                qobject_cast<QToolButton*>(wWidget) ||
                qobject_cast<QLineEdit*>(wWidget) ||
                qobject_cast<QSlider*>(wWidget)) {
                isInteractive = true;
                break;
            }
            wWidget = wWidget->parentWidget();
        }

        if (!isInteractive) {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                if (m_window->isMaximized()) m_window->showNormal();
                else m_window->showMaximized();
                return true;
            }
        }
    }

    return QObject::eventFilter(obj, event);
}

} // namespace QuarkMeta
