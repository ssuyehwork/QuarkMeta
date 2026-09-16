#ifndef DROPLISTVIEW_H
#define DROPLISTVIEW_H

#include <QTreeView>
#include <QHeaderView>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QStringList>
#include <QModelIndex>

namespace QuarkMeta {

class DropListView : public QTreeView {
    Q_OBJECT
public:
    explicit DropListView(QWidget* parent = nullptr);
    void setModel(QAbstractItemModel* model) override;

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void startDrag(Qt::DropActions supportedActions) override;

private:
    QModelIndex m_currentHoverDropIdx;
    void clearDropHighlight();

signals:
    void pathsDropped(const QStringList& paths, const QModelIndex& targetIndex);
    void blankSpaceDoubleClicked();

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
};

} // namespace QuarkMeta

#endif // DROPLISTVIEW_H
