#pragma once

#include <QWidget>
#include <QColor>

namespace QuarkMeta {

class ColorWheel : public QWidget {
    Q_OBJECT
public:
    explicit ColorWheel(QWidget* parent = nullptr);
    ~ColorWheel() override = default;

    int hue() const { return m_hue; }
    int precision() const { return m_precision; }

    void setValues(int hue, int precision);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void changed(int hue, int precision);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void handleMousePosition(const QPoint& pos, bool isRelease);
    int wheelRadius() const;

    int m_hue = 0;           // 0~359
    int m_precision = 50;    // 0~100 (0在边缘, 100在中心)
    bool m_isDragging = false;
};

} // namespace QuarkMeta
