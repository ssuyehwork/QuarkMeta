#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "FramelessWindowHelper.h"
#include "WindowStateController.h"
#include <QApplication>
#include <QDebug>
#include <QCoreApplication>
#include <QPushButton>
#include <QLineEdit>
#include <QToolButton>
#include <QSlider>
#include <QAbstractButton>
#include <QComboBox>
#include <QSpinBox>
#include <QScrollBar>
#include <QAbstractItemView>
#include <QMouseEvent>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

namespace QuarkMeta {

FramelessWindowHelper* FramelessWindowHelper::apply(QWidget* window, QWidget* titleBar, WindowStateController* windowStateController) {
    if (!window) return nullptr;
    return new FramelessWindowHelper(window, titleBar, windowStateController);
}

FramelessWindowHelper::FramelessWindowHelper(QWidget* window, QWidget* titleBar, WindowStateController* windowStateController)
    : QObject(window), m_window(window), m_titleBar(titleBar), m_windowStateController(windowStateController) {
    
    Qt::WindowFlags requiredFlags = m_window->windowFlags() | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint;
    if (m_window->windowFlags() != requiredFlags) {
        m_window->setWindowFlags(requiredFlags);
    }

    if (m_window) {
        m_window->setMouseTracking(true);
        if (QCoreApplication::instance()) {
            QCoreApplication::instance()->installEventFilter(this);
        }
    }
}

FramelessWindowHelper::~FramelessWindowHelper() {
    if (QCoreApplication::instance()) {
        QCoreApplication::instance()->removeEventFilter(this);
    }
}

bool FramelessWindowHelper::isInteractiveWidget(QWidget* child, QWidget* titleBar, QWidget* window) {
    QWidget* wWidget = child;
    while (wWidget && wWidget != titleBar && wWidget != window) {
        if (qobject_cast<QAbstractButton*>(wWidget) ||
            qobject_cast<QLineEdit*>(wWidget) ||
            qobject_cast<QSlider*>(wWidget) ||
            qobject_cast<QComboBox*>(wWidget) ||
            qobject_cast<QSpinBox*>(wWidget) ||
            qobject_cast<QScrollBar*>(wWidget) ||
            qobject_cast<QAbstractItemView*>(wWidget)) {
            return true;
        }
        wWidget = wWidget->parentWidget();
    }
    return false;
}

bool FramelessWindowHelper::handleNativeEvent(void* message, qintptr* result) {
#ifdef Q_OS_WIN
    if (!m_window || !result) return false;

    MSG* msg = static_cast<MSG*>(message);
    if (!msg) return false;

    // 1. 最大化多显示器边缘工作区补偿
    if (msg->message == WM_NCCALCSIZE) {
        if (msg->wParam == TRUE && m_window->isMaximized()) {
            NCCALCSIZE_PARAMS* pnc = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
            HMONITOR monitor = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST);
            if (monitor) {
                MONITORINFO monitorInfo = {};
                monitorInfo.cbSize = sizeof(MONITORINFO);
                if (GetMonitorInfo(monitor, &monitorInfo)) {
                    pnc->rgrc[0] = monitorInfo.rcWork;
                }
            }
        }
        *result = 0;
        return true;
    }

    if (msg->message == WM_GETMINMAXINFO) {
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(msg->lParam);
        if (mmi) {
            HMONITOR monitor = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST);
            if (monitor) {
                MONITORINFO monitorInfo = {};
                monitorInfo.cbSize = sizeof(MONITORINFO);
                if (GetMonitorInfo(monitor, &monitorInfo)) {
                    RECT workArea = monitorInfo.rcWork;
                    RECT monitorArea = monitorInfo.rcMonitor;
                    mmi->ptMaxPosition.x = workArea.left - monitorArea.left;
                    mmi->ptMaxPosition.y = workArea.top - monitorArea.top;
                    mmi->ptMaxSize.x = workArea.right - workArea.left;
                    mmi->ptMaxSize.y = workArea.bottom - workArea.top;
                }
            }
            if (m_window) {
                QSize minSz = m_window->minimumSize();
                if (minSz.width() > 0) mmi->ptMinTrackSize.x = minSz.width();
                if (minSz.height() > 0) mmi->ptMinTrackSize.y = minSz.height();
            }
        }
        *result = 0;
        return true;
    }

    // 2. 原生标题栏与四边四角缩放识别
    if (msg->message == WM_NCHITTEST) {
        if (m_window->isMaximized() || m_window->isFullScreen()) {
            *result = HTCLIENT;
            return true;
        }

        POINT screenPt = { GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam) };
        QPoint localPos = m_window->mapFromGlobal(QPoint(screenPt.x, screenPt.y));
        const int m = kBaseResizeMargin;
        const int w = m_window->width();
        const int h = m_window->height();

        bool onLeft   = localPos.x() <= m;
        bool onRight  = localPos.x() >= w - m;
        bool onTop    = localPos.y() <= m;
        bool onBottom = localPos.y() >= h - m;

        if (onTop && onLeft)     { *result = HTTOPLEFT;     return true; }
        if (onTop && onRight)    { *result = HTTOPRIGHT;    return true; }
        if (onBottom && onLeft)  { *result = HTBOTTOMLEFT;  return true; }
        if (onBottom && onRight) { *result = HTBOTTOMRIGHT; return true; }
        if (onLeft)              { *result = HTLEFT;        return true; }
        if (onRight)             { *result = HTRIGHT;       return true; }
        if (onTop)               { *result = HTTOP;         return true; }
        if (onBottom)            { *result = HTBOTTOM;      return true; }

        if (m_titleBar) {
            QRect titleRect = QRect(m_titleBar->mapTo(m_window, QPoint(0, 0)), m_titleBar->size());
            if (titleRect.contains(localPos)) {
                QWidget* childAtPt = m_window->childAt(localPos);
                if (!isInteractiveWidget(childAtPt, m_titleBar, m_window)) {
                    *result = HTCAPTION;
                    return true;
                }
            }
        }

        *result = HTCLIENT;
        return true;
    }

    // 3. 原生双击标题栏最大化 / 还原
    if (msg->message == WM_NCLBUTTONDBLCLK) {
        if (msg->wParam == HTCAPTION) {
            if (m_windowStateController) {
                m_windowStateController->toggleMaximizeRestore();
            }
            *result = 0;
            return true;
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
    return QObject::eventFilter(obj, event);
}

} // namespace QuarkMeta