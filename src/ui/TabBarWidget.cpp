#include "TabBarWidget.h"
#include "UiHelper.h"
#include <QVariant>

namespace QuarkMeta {

TabBarWidget::TabBarWidget(QWidget* parent)
    : QWidget(parent) {
    setFixedHeight(32);
    setObjectName("TabBarWidget");

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(4, 0, 4, 0);
    m_layout->setSpacing(4);

    m_tabBar = new QTabBar(this);
    m_tabBar->setTabsClosable(true);
    m_tabBar->setMovable(true);
    m_tabBar->setExpanding(false);
    m_tabBar->setDrawBase(false);
    m_tabBar->setObjectName("MainTabBar");

    m_btnNewTab = new QPushButton(this);
    m_btnNewTab->setFocusPolicy(Qt::NoFocus);
    m_btnNewTab->setFixedSize(24, 24);
    m_btnNewTab->setIcon(UiHelper::getIcon("add", QColor("#EEEEEE")));
    m_btnNewTab->setIconSize(QSize(16, 16));
    m_btnNewTab->setObjectName("NewTabBtn");

    m_layout->addWidget(m_tabBar);
    m_layout->addWidget(m_btnNewTab);
    m_layout->addStretch();

    connect(m_tabBar, &QTabBar::currentChanged, this, &TabBarWidget::currentChanged);
    connect(m_tabBar, &QTabBar::tabCloseRequested, this, &TabBarWidget::tabCloseRequested);
    connect(m_tabBar, &QTabBar::tabMoved, this, &TabBarWidget::tabMoved);
    connect(m_btnNewTab, &QPushButton::clicked, this, &TabBarWidget::newTabRequested);
}

int TabBarWidget::addTab(const QString& title, const QString& path) {
    int idx = m_tabBar->addTab(title);
    m_tabBar->setTabData(idx, path);
    return idx;
}

void TabBarWidget::setTabTitle(int index, const QString& title) {
    if (index >= 0 && index < m_tabBar->count()) {
        m_tabBar->setTabText(index, title);
    }
}

void TabBarWidget::setTabPath(int index, const QString& path) {
    if (index >= 0 && index < m_tabBar->count()) {
        m_tabBar->setTabData(index, path);
    }
}

QString TabBarWidget::tabPath(int index) const {
    if (index >= 0 && index < m_tabBar->count()) {
        return m_tabBar->tabData(index).toString();
    }
    return QString();
}

int TabBarWidget::currentIndex() const {
    return m_tabBar->currentIndex();
}

void TabBarWidget::setCurrentIndex(int index) {
    m_tabBar->setCurrentIndex(index);
}

int TabBarWidget::count() const {
    return m_tabBar->count();
}

void TabBarWidget::removeTab(int index) {
    if (index >= 0 && index < m_tabBar->count()) {
        m_tabBar->removeTab(index);
    }
}

} // namespace QuarkMeta
