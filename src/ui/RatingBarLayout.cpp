#include "RatingBarLayout.h"

namespace QuarkMeta {

RatingBarMetrics RatingBarLayout::calculate(const QRect& targetRect,
                                            RatingBarMode mode,
                                            int zoomLevel,
                                            int ratingH) {
    RatingBarMetrics m;
    m.banGap = 2; // 禁选与首星间距恒定为 2px

    if (mode == RatingBarMode::GridCard) {
        // 网格卡片模式参数
        m.starSize = 14;
        m.starSpacing = -2;
        m.banW = 12;

        if (zoomLevel < 100) {
            m.starSize = 12;
            m.starSpacing = -2;
            m.banW = 10;
        }

        m.totalWidth = m.banW + m.banGap + (5 * m.starSize) + (4 * m.starSpacing);
        int startX = targetRect.left() + (targetRect.width() - m.totalWidth) / 2;

        // 🚀【严格按约定接入 gap = 4】：星级条位于 cardRect.bottom() 下方 4px 处
        const int gap = 4;
        int ratingY = targetRect.bottom() + gap;
        m.centerY = ratingY + ratingH / 2;

        m.banRect = QRect(startX, m.centerY - m.banW / 2, m.banW, m.banW);
        m.starsStartX = startX + m.banW + m.banGap;

    } else { // TreeRow (列表表格第 2 列模式)
        m.starSize = 18;
        m.starSpacing = -4;
        m.banW = 12;

        m.totalWidth = m.banW + m.banGap + (5 * m.starSize) + (4 * m.starSpacing); // 88px
        int startX = targetRect.left() + (targetRect.width() - m.totalWidth) / 2;

        // 列表单元格直接在自身高度范围内绝对居中，不参与 gap 运算
        m.centerY = targetRect.top() + targetRect.height() / 2;

        m.banRect = QRect(startX, m.centerY - m.banW / 2, m.banW, m.banW);
        m.starsStartX = startX + m.banW + m.banGap;
    }

    return m;
}

} // namespace QuarkMeta
