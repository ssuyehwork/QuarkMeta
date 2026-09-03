#ifndef NOMINMAX
#define NOMINMAX
#endif
#pragma once

#include <QObject>
#include <QWidget>
#include <QPoint>
#include <QRect>
#include <QPointer>
#include <QAbstractNativeEventFilter>

namespace QuarkMeta {

class FramelessWindowHelper : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

public:
    static void apply(QWidget* window, QWidget* titleBar = nullptr);
    static void setAlwaysOnTop(QWidget* window, bool onTop);
    static bool isAlwaysOnTop(QWidget* window);

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    explicit FramelessWindowHelper(QWidget* window, QWidget* titleBar = nullptr);
    ~FramelessWindowHelper() override;

    QPointer<QWidget> m_window;
    QPointer<QWidget> m_titleBar;

    static constexpr int kBaseResizeMargin = 6;
};

} // namespace QuarkMeta
