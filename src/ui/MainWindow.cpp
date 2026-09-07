#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "MainWindow.h"
#include "TitleBarWidget.h"
#include "NavBarWidget.h"
#include "DriveBarWidget.h"
#include "UiHelper.h"
#include "SearchHistoryPanel.h"
#include "AppShortcutController.h"
#include "PanelMediator.h"
#include "SearchController.h"
#include "NavPanel.h"
#include "FavoritePanel.h"
#include "ContentPanel.h"
#include "MetaPanel.h"
#include "FilterPanel.h"
#include "TrayController.h"
#include "HoverEventFilter.h"
#include "FramelessWindowHelper.h"
#include "PanelLayoutManager.h"
#include "AddressBar.h"
#include "ToolTipOverlay.h"
#include "TaskProgressToolBar.h"
#include "../core/AppConfig.h"
#include "../core/NavigationService.h"
#include "../core/CoreController.h"
#include "../core/ModelContract.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QTimer>
#include <QCloseEvent>

namespace QuarkMeta {

constexpr int kLayoutEdgeMargin = 5;
constexpr int kStatusBarHorizontalMargin = 12;

MainWindow::~MainWindow() = default;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowMinMaxButtonsHint);
    setMinimumSize(475, 400);
    setWindowTitle("QuarkMeta");

    m_hoverFilter = new HoverEventFilter(this);
    m_isPinned = AppConfig::instance().getValue("MainWindow/AlwaysOnTop", false).toBool();

    ToolTipOverlay::instance();

    initUi();

    m_framelessHelper = FramelessWindowHelper::apply(this, m_titleBarWidget);
    if (m_isPinned) {
        FramelessWindowHelper::setAlwaysOnTop(this, true);
    }

    m_trayController = new TrayController(this);
    m_trayController->show();
}

void MainWindow::initUi() {
    QWidget* centralC = new QWidget(this);
    centralC->setObjectName("CentralWidget");
    QVBoxLayout* mainL = new QVBoxLayout(centralC);
    mainL->setContentsMargins(0, 0, 0, 0);
    mainL->setSpacing(0);

    QWidget* bodyWrapper = new QWidget(centralC);
    bodyWrapper->setObjectName("BodyWrapper");

    // Phase 1: 创建 Chrome 与面板 UI 控件
    setupChromeWidgets(centralC);
    setupPanelsAndSplitter(bodyWrapper);

    // Phase 2: 初始化控制器与信号槽绑定
    setupControllers();
    setupStatusBar(centralC);

    // Phase 3: 恢复几何状态与最大化状态（此时 m_titleBarWidget 等已安全创建并连接信号槽）
    QByteArray savedGeom = AppConfig::instance().getValue("MainWindow/Geometry").toByteArray();
    if (!savedGeom.isEmpty()) {
        restoreGeometry(savedGeom);
    } else {
        resize(1180, 800);
    }

    mainL->addWidget(m_titleBarWidget);
    mainL->addWidget(m_driveBarWidget);
    mainL->addWidget(m_navBarWidget);
    mainL->addWidget(bodyWrapper, 1);
    mainL->addWidget(m_statusBarWidget);
    mainL->addWidget(m_taskProgressToolBar);

    setCentralWidget(centralC);
}

void MainWindow::setupChromeWidgets(QWidget* centralC) {
    m_titleBarWidget = new TitleBarWidget(centralC, m_hoverFilter);
    m_navBarWidget   = new NavBarWidget(centralC, m_hoverFilter);
    m_driveBarWidget = new DriveBarWidget(centralC);

    m_addressBar       = m_navBarWidget->addressBar();
    m_searchController = m_navBarWidget->searchController();

    m_titleBarWidget->setInitialPinState(m_isPinned);

    connect(m_titleBarWidget, &TitleBarWidget::driveBarToggleRequested, this, [this](bool visible) {
        if (m_driveBarWidget) m_driveBarWidget->setVisible(visible);
    });
}

void MainWindow::setupPanelsAndSplitter(QWidget* bodyWrapper) {
    m_bodyLayout = new QVBoxLayout(bodyWrapper);
    m_bodyLayout->setContentsMargins(kLayoutEdgeMargin, 0, kLayoutEdgeMargin, kLayoutEdgeMargin);
    m_bodyLayout->setSpacing(0);

    m_mainSplitter = new QSplitter(Qt::Horizontal, bodyWrapper);
    m_mainSplitter->setHandleWidth(5);
    m_mainSplitter->setChildrenCollapsible(false);

    m_navPanel      = new NavPanel(this);      m_navPanel->setObjectName("SidebarContainer");
    m_favoritePanel = new FavoritePanel(this); m_favoritePanel->setObjectName("FavoriteContainer");
    m_contentPanel  = new ContentPanel(this);  m_contentPanel->setObjectName("EditorContainer");
    m_metaPanel     = new MetaPanel(this);     m_metaPanel->setObjectName("MetadataContainer");
    m_filterPanel   = new FilterPanel(this);   m_filterPanel->setObjectName("FilterContainer");

    m_mainSplitter->addWidget(m_navPanel);
    m_mainSplitter->addWidget(m_favoritePanel);
    m_mainSplitter->addWidget(m_contentPanel);
    m_mainSplitter->addWidget(m_metaPanel);
    m_mainSplitter->addWidget(m_filterPanel);

    m_bodyLayout->addWidget(m_mainSplitter);
}

void MainWindow::setupControllers() {
    m_panelLayoutManager = new PanelLayoutManager(this, m_mainSplitter, m_navPanel, m_favoritePanel, m_contentPanel, m_metaPanel, m_filterPanel, this);
    m_panelLayoutManager->initLayout();

    PanelMediatorComponents components;
    components.navPanel = m_navPanel;
    components.favoritePanel = m_favoritePanel;
    components.contentPanel = m_contentPanel;
    components.metaPanel = m_metaPanel;
    components.filterPanel = m_filterPanel;
    components.addressBar = m_addressBar;
    components.searchController = m_searchController;
    components.titleBar = m_titleBarWidget;
    components.layoutManager = m_panelLayoutManager;

    m_panelMediator = new PanelMediator(components, this);
    m_panelMediator->setupConnections();

    connect(m_panelMediator, &PanelMediator::statusMessageRequested, this, [this](const QString& msg) {
        if (m_statusLeft) m_statusLeft->setText(msg);
    });

    if (m_searchController) {
        m_searchController->bindContentPanel(m_contentPanel);
        connect(m_searchController, &SearchController::searchExecuted, this, &MainWindow::updateStatusBar);
    }

    m_shortcutController = new AppShortcutController(this, m_searchController, this);
    connect(m_shortcutController, &AppShortcutController::togglePinRequested, this, [this]() {
        if (m_titleBarWidget && m_titleBarWidget->btnPinTop()) {
            m_titleBarWidget->btnPinTop()->setChecked(!m_titleBarWidget->btnPinTop()->isChecked());
        }
    });
    connect(m_shortcutController, &AppShortcutController::toggleImmersiveRequested, this, [this]() {
        if (m_panelLayoutManager) m_panelLayoutManager->toggleImmersiveMode();
    });

    wireTitleBarSignals();
}

void MainWindow::wireTitleBarSignals() {
    // 置顶：持久化 + 窗口置顶操作，由 MainWindow 负责，TitleBarWidget 只发信号
    connect(m_titleBarWidget, &TitleBarWidget::pinToggled, this, [this](bool checked) {
        m_isPinned = checked;
        AppConfig::instance().setValue("MainWindow/AlwaysOnTop", checked);
        FramelessWindowHelper::setAlwaysOnTop(this, checked);
    });

    // 缩放滑杆 <-> ContentPanel 双向同步
    connect(m_titleBarWidget, &TitleBarWidget::zoomLevelRequested, this, [this](int level) {
        if (m_contentPanel) m_contentPanel->setZoomLevel(level);
    });
    connect(m_contentPanel, &ContentPanel::zoomLevelChanged, m_titleBarWidget, &TitleBarWidget::setZoomLevelDisplay);

    int initZoom = qBound(30, AppConfig::instance().getValue("UI/GridZoomLevel", 96).toInt(), 230);
    if (m_contentPanel) m_contentPanel->setZoomLevel(initZoom);
    m_titleBarWidget->setZoomLevelDisplay(initZoom);

    // 视图模式：TitleBarWidget 用 int 语义，这里做映射
    connect(m_titleBarWidget, &TitleBarWidget::viewModeRequested, this, [this](int mode) {
        if (!m_contentPanel) return;
        ContentPanel::ViewMode target = ContentPanel::JustifiedViewMode;
        if (mode == TitleBarWidget::GridViewOption) target = ContentPanel::GridView;
        else if (mode == TitleBarWidget::ListViewOption) target = ContentPanel::ListView;
        m_contentPanel->setViewMode(target);
        m_titleBarWidget->setCurrentViewMode(mode);
    });
    if (m_contentPanel) {
        ContentPanel::ViewMode cur = m_contentPanel->currentViewMode();
        int initMode = cur == ContentPanel::GridView ? TitleBarWidget::GridViewOption
                     : cur == ContentPanel::ListView ? TitleBarWidget::ListViewOption
                     : TitleBarWidget::AdaptiveView;
        m_titleBarWidget->setCurrentViewMode(initMode);
    }

    // 新建菜单
    connect(m_titleBarWidget, &TitleBarWidget::createItemRequested, this, [this](const QString& type) {
        if (m_contentPanel) m_contentPanel->createNewItem(type);
    });

    // 布局管理菜单
    connect(m_titleBarWidget, &TitleBarWidget::layoutMenuRequested, this, [this](const QPoint& pos) {
        if (m_panelLayoutManager) m_panelLayoutManager->showPanelContextMenu(pos);
    });
}

void MainWindow::setupStatusBar(QWidget* centralC) {
    m_statusBarWidget = new QWidget(centralC);
    m_statusBarWidget->setObjectName("StatusBar");
    m_statusBarWidget->setFixedHeight(32);
    QHBoxLayout* statusL = new QHBoxLayout(m_statusBarWidget);
    statusL->setContentsMargins(kStatusBarHorizontalMargin, 0, kStatusBarHorizontalMargin, 0);
    statusL->setSpacing(0);

    m_statusLeft = new QLabel("就绪中...", m_statusBarWidget);
    m_statusLeft->setObjectName("StatusBarLeft");
    statusL->addWidget(m_statusLeft);
    statusL->addStretch(1);

    connect(m_contentPanel, &ContentPanel::statusBarMessageReady, this, [this](const QString& msg) {
        if (m_statusLeft) m_statusLeft->setText(msg);
    });

    auto updateStatus = [this]() {
        m_statusLeft->setText(CoreController::instance().statusText());
        m_statusLeft->setProperty("indexing", CoreController::instance().isIndexing());
        m_statusLeft->style()->unpolish(m_statusLeft);
        m_statusLeft->style()->polish(m_statusLeft);
    };
    connect(&CoreController::instance(), &CoreController::statusTextChanged, this, updateStatus);
    connect(&CoreController::instance(), &CoreController::isIndexingChanged, this, updateStatus);
    updateStatus();

    m_taskProgressToolBar = new TaskProgressToolBar(centralC);
    m_taskProgressToolBar->hide();
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (!m_panelsInitialized) {
        m_panelsInitialized = true;
        if (m_navPanel) m_navPanel->deferredInit();

        QString lastPath = AppConfig::instance().getValue("MainWindow/LastPath", "computer://").toString();
        bool isValid = lastPath.contains("://") || QDir(lastPath).exists();
        NavigationService::instance().navigateTo(isValid ? lastPath : "computer://");

        QTimer::singleShot(500, []() {
            ToolTipOverlay::instance()->silentWarmup();
        });
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    setAttribute(Qt::WA_Hover);
    QMainWindow::keyPressEvent(event);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::onStatusBarStatsUpdated(int fileCount, int folderCount, int totalCount) {
    Q_UNUSED(fileCount); Q_UNUSED(folderCount); Q_UNUSED(totalCount);
}

void MainWindow::updateStatusBar() {
    onStatusBarStatsUpdated(0, 0, 0);
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() && m_searchController && m_searchController->historyPanel()) {
            m_searchController->historyPanel()->hide();
        }
        if (m_titleBarWidget) {
            m_titleBarWidget->setWindowMaximized(isMaximized());
        }
        if (m_bodyLayout) {
            m_bodyLayout->setContentsMargins(kLayoutEdgeMargin, 0, kLayoutEdgeMargin, kLayoutEdgeMargin);
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    AppConfig::instance().setValue("MainWindow/LastPath", NavigationService::instance().currentUrl());
    AppConfig::instance().setValue("MainWindow/Geometry", saveGeometry());
    if (m_panelLayoutManager) {
        m_panelLayoutManager->saveLayoutState();
    }
    AppConfig::instance().sync();
    QMainWindow::closeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
}

bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    if (m_framelessHelper && m_framelessHelper->handleNativeEvent(message, result)) {
        return true;
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}

} // namespace QuarkMeta
