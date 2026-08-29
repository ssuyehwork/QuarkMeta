#include "RatingBarLayout.h"

namespace QuarkMeta {

RatingBarMetrics RatingBarLayout::calculate(const QRect& targetRect,
                                            RatingBarMode mode,
                                            int zoomLevel,
                                            int ratingH) {
    RatingBarMetrics m;
    m.banGap = 4; // 禁选 ⊘ 与首星间距设为舒适的 4px

    if (mode == RatingBarMode::GridCard) {
        // 🚀【核心修正】：星星尺寸 13px，星间距设为正数 +2px，彻底消除重叠挤压！
        m.starSize = 13;
        m.starSpacing = 2;
        m.banW = 12;

        if (zoomLevel < 100) {
            m.starSize = 11;
            m.starSpacing = 1;
            m.banW = 10;
        }

        m.totalWidth = m.banW + m.banGap + (5 * m.starSize) + (4 * m.starSpacing);
        int startX = targetRect.left() + (targetRect.width() - m.totalWidth) / 2;

        const int gap = 4;
        int ratingY = targetRect.bottom() + gap;
        m.centerY = ratingY + ratingH / 2;

        m.banRect = QRect(startX, m.centerY - m.banW / 2, m.banW, m.banW);
        m.starsStartX = startX + m.banW + m.banGap;

    } else { // TreeRow (列表模式)
        m.starSize = 16;
        m.starSpacing = 2;
        m.banW = 12;
        m.banGap = 4;

        m.totalWidth = m.banW + m.banGap + (5 * m.starSize) + (4 * m.starSpacing);
        int startX = targetRect.left() + (targetRect.width() - m.totalWidth) / 2;

        m.centerY = targetRect.top() + targetRect.height() / 2;

        m.banRect = QRect(startX, m.centerY - m.banW / 2, m.banW, m.banW);
        m.starsStartX = startX + m.banW + m.banGap;
    }

    return m;
}

} // namespace QuarkMeta
