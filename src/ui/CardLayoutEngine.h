#pragma once

#include <QRect>
#include <QPoint>
#include <algorithm>

namespace QuarkMeta {

/**
 * @brief 归一化卡片几何结构体
 */
struct CardLayout {
    QRect totalRect;       // 单元格总矩形
    QRect coverRect;       // 缩略图主体区域
    QRect badgeRect;       // 扩展名徽章 (DIR/FILE)
    QRect pinRect;         // 置顶图标
    QRect capsuleRect;     // 底部色标星级胶囊条
    QRect banRect;         // ⊘ 禁选按钮
    QRect starRects[5];    // 5 颗星各自精确矩形 (自带 5px 呼吸间距)
    QRect textRect;        // 文件名区域 (最多2行)

    // 零误差交互命中查询：返回 1~5 (星级), 0 (禁选⊘), -1 (未命中)
    int hitStar(const QPoint& pt) const {
        if (banRect.contains(pt)) return 0;
        for (int i = 0; i < 5; ++i) {
            if (starRects[i].contains(pt)) return i + 1;
        }
        return -1;
    }

    bool isTextHit(const QPoint& pt) const { return textRect.contains(pt); }
    bool isCoverHit(const QPoint& pt) const { return coverRect.contains(pt); }
};

class CardLayoutEngine {
public:
    /**
     * @brief 统一计算卡片全要素几何坐标
     * @param totalRect 单元格总分配区域
     * @param zoomLevel 当前缩放级别
     */
    static CardLayout calculate(const QRect& totalRect, int zoomLevel = 96);
};

} // namespace QuarkMeta
