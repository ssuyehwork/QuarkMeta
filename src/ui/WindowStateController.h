#pragma once

#include <QObject>
#include <QRect>
#include <QByteArray>
#include <QPointer>

class QMainWindow;
class QSplitter;
class QMenu;

namespace QuarkMeta {

class NavPanel;
class FavoritePanel;
class ContentPanel;
class MetaPanel;
class FilterPanel;

/**
 * @brief 统一窗口状态控制器：收拢几何记忆、最大化/还原、分栏布局状态的读写与时序
 *
 * 取代原先分散在 MainWindow / FramelessWindowHelper / PanelLayoutManager 里的
 * 状态存取逻辑。所有跟"窗口应该长什么样、什么时候恢复成什么样"相关的决策都在这里做。
 */
class WindowStateController : public QObject {
    Q_OBJECT
public:
    explicit WindowStateController(QMainWindow* mainWindow,
                                    QSplitter* mainSplitter,
                                    NavPanel* navPanel,
                                    FavoritePanel* favoritePanel,
                                    ContentPanel* contentPanel,
                                    MetaPanel* metaPanel,
                                    FilterPanel* filterPanel,
                                    QObject* parent = nullptr);
    ~WindowStateController() override = default;

    // === 阶段一：构造期，窗口 show() 之前调用 ===
    // 只做不依赖子部件实际尺寸的事：读取并 restoreGeometry、设置初始 stretchFactor
    void applyPreShowState();

    // === 阶段二：窗口首次真实布局完成后调用 ===
    // 此时 splitter 已有真实宽度，可以安全做比例换算
    void applyPostLayoutState();

    // === 最大化/还原：替代原先分散在 TitleBarWidget / FramelessWindowHelper 里的直接调用 ===
    void toggleMaximizeRestore();

    // === 供 FramelessWindowHelper 双击标题栏复用 ===
    void requestMaximize();
    void requestRestore();

    // 供 FramelessWindowHelper 在窗口状态变化时上报，用于自行记账 m_lastNormalGeometry
    void notifyAboutToMaximize();

    // === 关闭时统一保存所有状态（几何 + 分栏 + 面板可见性）===
    void saveAllState();

    // 分栏 / 沉浸模式相关：从 PanelLayoutManager 迁移过来的对外接口保持不变
    void resetSplitterLayout();
    void setPanelVisible(const QString& panelId, bool visible);
    bool isPanelVisible(const QString& panelId) const;
    bool isImmersiveMode() const;
    void toggleImmersiveMode();
    void populatePanelMenu(QMenu* menu);
    void showPanelContextMenu(const QPoint& globalPos);

signals:
    void panelVisibilityChanged(const QString& panelId, bool visible);
    void layoutResetCompleted();

private:
    void updateDynamicMinimumSize();
    void savePreImmersiveState();
    void restorePreImmersiveState();

    QPointer<QMainWindow> m_mainWindow;
    QPointer<QSplitter> m_mainSplitter;
    QPointer<NavPanel> m_navPanel;
    QPointer<FavoritePanel> m_favoritePanel;
    QPointer<ContentPanel> m_contentPanel;
    QPointer<MetaPanel> m_metaPanel;
    QPointer<FilterPanel> m_filterPanel;

    // 自己记账的"正常几何"，不依赖 Qt normalGeometry() 或 Win32 rcNormalPosition
    QRect m_lastNormalGeometry;

    static constexpr int kBasePanelWidth = 230;
    static constexpr int kContentBaseWidth = 230;
    static constexpr int kSplitterHandleWidth = 5;
    static constexpr int kWindowAbsoluteMinWidth = 475;
};

} // namespace QuarkMeta
