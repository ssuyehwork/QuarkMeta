#include "CardLayoutEngine.h"

namespace QuarkMeta {

CardLayout CardLayoutEngine::calculate(const QRect& totalRect, int zoomLevel) {
    CardLayout l;
    l.totalRect = totalRect;

    const int padding = 3;
    const int textHeight = 36;
    const int ratingH = 22;
    const int gap = 4;

    // 1. Cover 区域
    int coverBottom = totalRect.bottom() - (textHeight + ratingH + gap + padding);
    l.coverRect = QRect(totalRect.left() + padding,
                        totalRect.top() + padding,
                        totalRect.width() - padding * 2,
                        coverBottom - (totalRect.top() + padding));

    // 2. 徽章与置顶
    l.badgeRect = QRect(l.coverRect.left() + 4, l.coverRect.top() + 4, 30, 16);
    l.pinRect = QRect(l.coverRect.right() - 20, l.coverRect.top() + 4, 16, 16);

    // 3. Footer 胶囊条 (色标底色 + ⊘ + 5 星)
    int ratingY = l.coverRect.bottom() + gap;
    l.capsuleRect = QRect(totalRect.left() + padding, ratingY, totalRect.width() - padding * 2, ratingH);

    // 4. 星星与禁选按钮布局 (带 5px 呼吸间距)
    int starSize = (zoomLevel < 100) ? 11 : 13;
    int starSpacing = (zoomLevel < 100) ? 4 : 5;
    int banW = (zoomLevel < 100) ? 10 : 12;
    int banGap = 5;

    int totalBarContentW = banW + banGap + (5 * starSize) + (4 * starSpacing);
    int startX = l.capsuleRect.left() + (l.capsuleRect.width() - totalBarContentW) / 2;
    int centerY = l.capsuleRect.top() + l.capsuleRect.height() / 2;

    l.banRect = QRect(startX, centerY - banW / 2, banW, banW);

    int starsStartX = startX + banW + banGap;
    for (int i = 0; i < 5; ++i) {
        l.starRects[i] = QRect(starsStartX + i * (starSize + starSpacing),
                               centerY - starSize / 2,
                               starSize, starSize);
    }

    // 5. 文本区域
    l.textRect = QRect(totalRect.left() + padding,
                       l.capsuleRect.bottom() + 2,
                       totalRect.width() - padding * 2,
                       textHeight);

    return l;
}

} // namespace QuarkMeta
