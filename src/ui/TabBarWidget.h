#pragma once

#include <QWidget>
#include <QTabBar>
#include <QPushButton>
#include <QHBoxLayout>

namespace QuarkMeta {

class TabBarWidget : public QWidget {
    Q_OBJECT

public:
    explicit TabBarWidget(QWidget* parent = nullptr);
    ~TabBarWidget() override = default;

    int addTab(const QString& title, const QString& path);
    void setTabTitle(int index, const QString& title);
    void setTabPath(int index, const QString& path);
    QString tabPath(int index) const;

    int currentIndex() const;
    void setCurrentIndex(int index);
    int count() const;
    void removeTab(int index);

signals:
    void currentChanged(int index);
    void tabCloseRequested(int index);
    void newTabRequested();

private:
    QHBoxLayout* m_layout = nullptr;
    QTabBar* m_tabBar = nullptr;
    QPushButton* m_btnNewTab = nullptr;
};

} // namespace QuarkMeta
