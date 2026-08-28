#pragma once

#include <QObject>
#include <QPoint>
#include <QAbstractItemView>

namespace QuarkMeta {

class ContentPanel;

class ContentContextMenuHandler : public QObject {
    Q_OBJECT
public:
    explicit ContentContextMenuHandler(ContentPanel* panel);

    void showContextMenu(QAbstractItemView* view, const QPoint& pos);

private:
    ContentPanel* m_panel = nullptr;
};

} // namespace QuarkMeta
