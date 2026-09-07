#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>

namespace QuarkMeta {

class HoverEventFilter;

/**
 * @brief 独立标题栏组件
 * 只负责 UI 渲染与信号发射，不持有 ContentPanel/PanelLayoutManager/AppConfig 等任何业务或持久化对象。
 * 所有业务联动由外部（MainWindow）负责连接。
 */
class TitleBarWidget : public QWidget {
    Q_OBJECT

public:
    // 与 ContentPanel::ViewMode 语义对应，由外部负责映射，避免本类依赖 ContentPanel 头文件
    enum ViewModeOption {
        AdaptiveView = 0,
        GridViewOption = 1,
        ListViewOption = 2
    };

    explicit TitleBarWidget(QWidget* parent = nullptr, HoverEventFilter* hoverFilter = nullptr);
    ~TitleBarWidget() override = default;

    QPushButton* btnPinTop() const { return m_btnPinTop; }
    QPushButton* btnMin() const { return m_btnMin; }
    QPushButton* btnMax() const { return m_btnMax; }
    QPushButton* btnClose() const { return m_btnClose; }
    QPushButton* btnToggleDriveBar() const { return m_btnToggleDriveBar; }
    QPushButton* btnLayout() const { return m_btnLayout; }
    QPushButton* btnCreate() const { return m_btnCreate; }
    QPushButton* btnViewMenu() const { return m_btnViewMenu; }
    QSlider* sizeSlider() const { return m_sizeSlider; }

public slots:
    void setInitialPinState(bool pinned);
    void setZoomLevelDisplay(int level); // 外部在 ContentPanel::zoomLevelChanged 时回写，不触发二次请求
    void setCurrentViewMode(int mode);   // 外部同步当前视图模式，供菜单勾选态使用
    void setWindowMaximized(bool maximized); // 驱动接口：由 MainWindow 在窗口状态（最大化/还原）改变时统一调用

signals:
    void driveBarToggleRequested(bool visible);
    void pinToggled(bool pinned);
    void zoomLevelRequested(int level);
    void viewModeRequested(int mode);
    void createItemRequested(const QString& type);
    void layoutMenuRequested(const QPoint& globalPos);

private:
    void initUi(HoverEventFilter* hoverFilter);
    void setupViewMenu();
    void setupCreateMenu();

    QHBoxLayout* m_layout = nullptr;
    QLabel* m_logoLabel = nullptr;
    QLabel* m_appNameLabel = nullptr;

    QPushButton* m_btnViewMenu = nullptr;
    QSlider* m_sizeSlider = nullptr;

    QPushButton* m_btnToggleDriveBar = nullptr;
    QPushButton* m_btnLayout = nullptr;
    QPushButton* m_btnCreate = nullptr;
    QPushButton* m_btnPinTop = nullptr;
    QPushButton* m_btnMin = nullptr;
    QPushButton* m_btnMax = nullptr;
    QPushButton* m_btnClose = nullptr;

    int m_currentViewMode = AdaptiveView;
    bool m_isPinned = false;
};

} // namespace QuarkMeta
