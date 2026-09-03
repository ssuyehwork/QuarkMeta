#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "FramelessWindowHelper.h"
#include <QMouseEvent>
#include <QApplication>
#include <QCoreApplication>
#include <QPushButton>
#include <QLineEdit>
#include <QToolButton>
#include <QSlider>

#ifdef Q_OS_WIN
#include <windows.h>
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
    
    // 安装至全局应用事件总线，处理标题栏双击交互
    QCoreApplication::instance()->installEventFilter(this);
}

FramelessWindowHelper::~FramelessWindowHelper() = default;

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

    // 仅拦截属于当前窗口及其子控件树的事件
    QWidget* widget = qobject_cast<QWidget*>(obj);
    if (!widget || (widget != m_window && !m_window->isAncestorOf(widget))) {
        return false;
    }

    // 标题栏双击最大化与还原交互（拖拽与拉伸缩放已由 WM_NCHITTEST 原生接管）
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
