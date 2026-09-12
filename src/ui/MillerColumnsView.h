#pragma once

#include <QAbstractItemView>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QListView>
#include <QList>

namespace QuarkMeta {

class DiskItemModel;
class FilterProxyModel;

class MillerColumnPane : public QWidget {
    Q_OBJECT

public:
    explicit MillerColumnPane(const QString& path, QWidget* parent = nullptr);
    ~MillerColumnPane() override = default;

    QString path() const { return m_path; }
    QListView* listView() const { return m_listView; }
    DiskItemModel* model() const { return m_diskModel; }
    FilterProxyModel* proxyModel() const { return m_proxyModel; }

signals:
    void folderSelected(const QString& folderPath);
    void fileSelected(const QString& filePath);
    void fileDoubleClicked(const QString& filePath);

private:
    void initPane();

    QString m_path;
    QListView* m_listView = nullptr;
    DiskItemModel* m_diskModel = nullptr;
    FilterProxyModel* m_proxyModel = nullptr;
};

class MillerColumnsView : public QAbstractItemView {
    Q_OBJECT

public:
    explicit MillerColumnsView(QWidget* parent = nullptr);
    ~MillerColumnsView() override = default;

    void setRootPath(const QString& rootPath);
    QString rootPath() const { return m_rootPath; }

    // QAbstractItemView virtual implementations
    QRect visualRect(const QModelIndex& index) const override;
    void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible) override;
    QModelIndex indexAt(const QPoint& point) const override;
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override;
    int horizontalOffset() const override;
    int verticalOffset() const override;
    bool isIndexHidden(const QModelIndex& index) const override;
    void setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags command) override;
    QRegion visualRegionForSelection(const QItemSelection& selection) const override;

signals:
    void fileActivated(const QString& path);
    void fileSelected(const QString& path);
    void directoryNavigated(const QString& path);

private:
    void appendColumn(const QString& folderPath, int parentPaneIndex);
    void truncateColumnsAfter(int paneIndex);
    void scrollToRightmostPane();

    QString m_rootPath;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_containerWidget = nullptr;
    QHBoxLayout* m_containerLayout = nullptr;
    QList<MillerColumnPane*> m_panes;
};

} // namespace QuarkMeta
