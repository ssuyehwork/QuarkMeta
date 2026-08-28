#include "ContentContextMenuHandler.h"
#include "../ContentPanel.h"

namespace QuarkMeta {

ContentContextMenuHandler::ContentContextMenuHandler(ContentPanel* panel)
    : QObject(panel), m_panel(panel) {}

void ContentContextMenuHandler::showContextMenu(QAbstractItemView* view, const QPoint& pos) {
    Q_UNUSED(pos);
    if (!m_panel || !view) return;
    // Context menu delegation handler
}

} // namespace QuarkMeta
