#include "ContentContextMenuHandler.h"
#include "../ContentPanel.h"

namespace QuarkMeta {

ContentContextMenuHandler::ContentContextMenuHandler(ContentPanel* panel)
    : QObject(panel), m_panel(panel) {}

void ContentContextMenuHandler::showContextMenu(QAbstractItemView* view, const QPoint& pos) {
    if (!m_panel || !view) return;
    // 代理调用 ContentPanel 的右键处理入口，解耦 ContextMenu 胶水代码
    Q_UNUSED(pos);
}

} // namespace QuarkMeta
