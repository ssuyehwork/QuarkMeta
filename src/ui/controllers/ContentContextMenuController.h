#pragma once

#include <QObject>
#include <QPoint>
#include <QAbstractItemView>
#include <QWidget>

namespace QuarkMeta {

class ContentPanel;

class ContentContextMenuController : public QObject {
    Q_OBJECT

public:
    explicit ContentContextMenuController(ContentPanel* panel, QObject* parent = nullptr);
    ~ContentContextMenuController() override = default;

    /**
     * @brief 弹出并处理内容区右键上下文菜单 (全场景自动化分流)
     */
    void showContextMenu(QAbstractItemView* view,
                          const QPoint& pos,
                          const QString& currentPath,
                          const QString& categoryType);

private:
    ContentPanel* m_panel = nullptr;
};

} // namespace QuarkMeta
