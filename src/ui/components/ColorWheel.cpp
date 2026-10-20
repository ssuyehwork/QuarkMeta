#include "ColorWheel.h"
#include "../ToolTipOverlay.h"
#include <QPainter>
#include <QPainterPath>
#include <QConicalGradient>
#include <QRadialGradient>
#include <QMouseEvent>
#include <QtMath>

namespace QuarkMeta {

ColorWheel::ColorWheel(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

QSize ColorWheel::sizeHint() const {
    return QSize(160, 160);
}

QSize ColorWheel::minimumSizeHint() const {
    return QSize(80, 80);
}

int ColorWheel::wheelRadius() const {
    int side = qMin(width() - 16, 160);
    side = qMax(side, 40);
    return side / 2 - 6; // 留 6px 描边与点 C 空间
}

void ColorWheel::setValues(int hue, int precision) {
    int newHue = qBound(0, hue, 359);
    int newPrecision = qBound(0, precision, 100);
    if (m_hue != newHue || m_precision != newPrecision) {
        m_hue = newHue;
        m_precision = newPrecision;
        update();
    }
}

void ColorWheel::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    int side = qMin(width() - 16, 160);
    side = qMax(side, 40);

    QPointF center(width() / 2.0, height() / 2.0);
    double radius = side / 2.0 - 6.0;
    if (radius <= 0) return;

    // 1. 在圆形剪裁区域绘制色轮
    QPainterPath clipPath;
    clipPath.addEllipse(center, radius, radius);

    p.save();
    p.setClipPath(clipPath);

    // 使用 QConicalGradient 绘制色相 (0° 红色在右，逆时针递增)
    QConicalGradient conicalGrad(center, 0);
    conicalGrad.setColorAt(0.0,         QColor(255, 0, 0));
    conicalGrad.setColorAt(60.0 / 360,  QColor(255, 255, 0));
    conicalGrad.setColorAt(120.0 / 360, QColor(0, 255, 0));
    conicalGrad.setColorAt(180.0 / 360, QColor(0, 255, 255));
    conicalGrad.setColorAt(240.0 / 360, QColor(0, 0, 255));
    conicalGrad.setColorAt(300.0 / 360, QColor(255, 0, 255));
    conicalGrad.setColorAt(1.0,         QColor(255, 0, 0));

    // Qt 默认在屏坐标系 (Y向下) 下 QConicalGradient 顺时针旋转。
    // 将原点平移到 center，翻转 Y 轴即可使角度变为物理逆时针！
    p.translate(center);
    p.scale(1, -1);

    // 在变换后坐标系（原点在 (0,0)）下绘制色相环
    QConicalGradient localConical(QPointF(0, 0), 0);
    localConical.setStops(conicalGrad.stops());
    p.setPen(Qt::NoPen);
    p.setBrush(localConical);
    p.drawRect(QRectF(-radius, -radius, radius * 2, radius * 2));

    // 叠加由中心灰(#B0B0B0)向边缘透明的径向渐变
    QRadialGradient radialGrad(QPointF(0, 0), radius);
    radialGrad.setColorAt(0.0, QColor("#B0B0B0"));
    radialGrad.setColorAt(1.0, QColor(176, 176, 176, 0));
    p.setBrush(radialGrad);
    p.drawRect(QRectF(-radius, -radius, radius * 2, radius * 2));

    p.restore();

    // 绘制色轮外圈细黑/灰保护边线
    p.setPen(QPen(QColor(60, 60, 60), 1));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(center, radius, radius);

    // 2. 绘制点 C
    double r = (1.0 - double(m_precision) / 100.0) * radius;
    double rad = qDegreesToRadians(double(m_hue));
    double cx = center.x() + r * qCos(rad);
    double cy = center.y() - r * qSin(rad); // 屏坐标系 Y 向下

    QPointF dotCenter(cx, cy);

    // 计算点 C 的填充颜色 (中心灰色 #B0B0B0 向边缘纯色过渡)
    double factor = (100.0 - double(m_precision)) / 100.0;
    QColor pureCol = QColor::fromHsv(m_hue, 255, 255);
    QColor grayCol = QColor("#B0B0B0");
    QColor cFill(
        grayCol.red() + qRound(factor * (pureCol.red() - grayCol.red())),
        grayCol.green() + qRound(factor * (pureCol.green() - grayCol.green())),
        grayCol.blue() + qRound(factor * (pureCol.blue() - grayCol.blue()))
    );

    // 绘制白色描边 + 实时色彩填充的小圆点
    p.setPen(QPen(Qt::white, 1.5));
    p.setBrush(cFill);
    p.drawEllipse(dotCenter, 6.0, 6.0);
}

void ColorWheel::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        handleMousePosition(event->pos(), false);
        event->accept();
    } else {
        QWidget::mousePressEvent(event);
    }
}

void ColorWheel::mouseMoveEvent(QMouseEvent* event) {
    if (m_isDragging) {
        handleMousePosition(event->pos(), false);
        event->accept();
    } else {
        QWidget::mouseMoveEvent(event);
    }
}

void ColorWheel::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_isDragging) {
        m_isDragging = false;
        ToolTipOverlay::hideTip();
        handleMousePosition(event->pos(), true);
        event->accept();
    } else {
        QWidget::mouseReleaseEvent(event);
    }
}

void ColorWheel::handleMousePosition(const QPoint& pos, bool isRelease) {
    int side = qMin(width() - 16, 160);
    side = qMax(side, 40);

    QPointF center(width() / 2.0, height() / 2.0);
    double radius = side / 2.0 - 6.0;
    if (radius <= 0) return;

    double dx = pos.x() - center.x();
    double dy = center.y() - pos.y(); // 笛卡尔坐标系 Y 向上
    double dist = qSqrt(dx * dx + dy * dy);

    if (dist > radius) {
        dist = radius;
    }

    if (dist > 0.001) {
        double rad = qAtan2(dy, dx);
        double deg = qRadiansToDegrees(rad);
        if (deg < 0) deg += 360.0;
        m_hue = qRound(deg) % 360;
    }

    m_precision = qBound(0, qRound(100.0 * (1.0 - dist / radius)), 100);
    update();

    if (!isRelease) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("精准度: %1").arg(m_precision), 0);
    } else {
        emit changed(m_hue, m_precision);
    }
}

} // namespace QuarkMeta
