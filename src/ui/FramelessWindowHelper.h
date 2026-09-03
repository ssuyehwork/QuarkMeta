#ifndef NOMINMAX
#define NOMINMAX
#endif
#pragma once

#include <QObject>
#include <QWidget>
#include <QPointer>

namespace QuarkMeta {

class FramelessWindowHelper : public QObject {
    Q_OBJECT

public:
    static void apply(QWidget* window, QWidget* titleBar = nullptr);
    static void setAlwaysOnTop(QWidget* window, bool onTop);
    static bool isAlwaysOnTop(QWidget* window);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    explicit FramelessWindowHelper(QWidget* window, QWidget* titleBar = nullptr);
    ~FramelessWindowHelper() override;

    QPointer<QWidget> m_window;
    QPointer<QWidget> m_titleBar;
};

} // namespace QuarkMeta
