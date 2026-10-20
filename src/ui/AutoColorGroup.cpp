#include "AutoColorGroup.h"
#include "components/ColorWheel.h"
#include "components/ColorBlock.h"
#include "components/FlowLayout.h"
#include "ToolTipOverlay.h"
#include "AppConfig.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QTimer>
#include <QEvent>
#include <QMouseEvent>
#include <QCursor>

namespace QuarkMeta {

namespace {

class SliderToolTipFilter : public QObject {
public:
    explicit SliderToolTipFilter(QObject* parent = nullptr) : QObject(parent) {}

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        QSlider* slider = qobject_cast<QSlider*>(watched);
        if (!slider) return false;

        switch (event->type()) {
            case QEvent::MouseMove:
            case QEvent::MouseButtonPress: {
                QPoint globalPos = QCursor::pos();
                ToolTipOverlay::instance()->showText(globalPos, QString("%1%").arg(slider->value()), 0);
                break;
            }
            case QEvent::MouseButtonRelease:
            case QEvent::Leave: {
                ToolTipOverlay::hideTip();
                break;
            }
            default:
                break;
        }
        return false;
    }
};

} // namespace

void AutoColorGroup::pushRecent(const QString& hex) {
    if (hex.isEmpty()) return;
    QString hexUpper = hex.toUpper();
    if (!hexUpper.startsWith('#')) {
        hexUpper = "#" + hexUpper;
    }

    QStringList recent = AppConfig::instance().getValue("Filter/RecentColors").toStringList();
    recent.removeAll(hexUpper);
    recent.prepend(hexUpper);
    while (recent.size() > 50) {
        recent.removeLast();
    }
    AppConfig::instance().setValue("Filter/RecentColors", recent);
}

void AutoColorGroup::populate(QWidget* parentWidget,
                             QVBoxLayout* contentLayout,
                             FilterStateModel* filterModel,
                             std::function<void()> requestRebuild) {
    if (!contentLayout || !filterModel) return;

    QWidget* container = new QWidget(parentWidget ? parentWidget : contentLayout->parentWidget());
    container->setObjectName("AutoColorGroupContainer");
    QVBoxLayout* groupLayout = new QVBoxLayout(container);
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->setSpacing(4);

    const FilterState currentState = filterModel->state();

    // 1. 色轮 ColorWheel 居中布局
    QWidget* wheelContainer = new QWidget(container);
    QHBoxLayout* wheelLayout = new QHBoxLayout(wheelContainer);
    wheelLayout->setContentsMargins(8, 4, 8, 4);
    wheelLayout->setSpacing(0);

    ColorWheel* colorWheel = new ColorWheel(wheelContainer);
    colorWheel->setValues(currentState.colorHue, currentState.colorPrecision);
    wheelLayout->addWidget(colorWheel, 0, Qt::AlignCenter);
    groupLayout->addWidget(wheelContainer);

    // 2. "占比:" 行
    QWidget* areaRow = new QWidget(container);
    QHBoxLayout* areaLayout = new QHBoxLayout(areaRow);
    areaLayout->setContentsMargins(10, 4, 10, 4);
    areaLayout->setSpacing(8);

    QLabel* areaLabel = new QLabel("占比:", areaRow);
    areaLabel->setObjectName("FilterColorSliderLabel");
    QSlider* areaSlider = new QSlider(Qt::Horizontal, areaRow);
    areaSlider->setObjectName("FilterColorSlider");
    areaSlider->setRange(0, 100);
    areaSlider->setValue(currentState.minColorArea);

    areaLayout->addWidget(areaLabel);
    areaLayout->addWidget(areaSlider, 1);
    groupLayout->addWidget(areaRow);

    // 色轮松手响应
    QObject::connect(colorWheel, &ColorWheel::changed, container, [filterModel, colorWheel, areaSlider, requestRebuild](int hue, int precision) {
        QColor col = QColor::fromHsv(hue, 255, 255);
        QString hex = col.name().toUpper();
        pushRecent(hex);

        int area = areaSlider->value();
        AppConfig::instance().setValue("Filter/ColorHue", hue);
        AppConfig::instance().setValue("Filter/ColorPrecision", precision);
        AppConfig::instance().setValue("Filter/MinColorArea", area);

        FilterState st = filterModel->state();
        st.colors = QStringList{ hex };
        st.colorHue = hue;
        st.colorPrecision = precision;
        st.minColorArea = area;
        filterModel->setState(st);

        if (requestRebuild) requestRebuild();
    });

    // 占比防抖 100ms 定时器
    QTimer* debounceTimer = new QTimer(container);
    debounceTimer->setSingleShot(true);
    debounceTimer->setInterval(100);

    QObject::connect(areaSlider, &QSlider::valueChanged, container, [debounceTimer]() {
        debounceTimer->start();
    });

    QObject::connect(debounceTimer, &QTimer::timeout, container, [filterModel, areaSlider]() {
        int area = areaSlider->value();
        AppConfig::instance().setValue("Filter/MinColorArea", area);

        FilterState st = filterModel->state();
        st.minColorArea = area;
        filterModel->setState(st);
    });

    // 占比滑条事件过滤器（ToolTip）
    SliderToolTipFilter* tipFilter = new SliderToolTipFilter(container);
    areaSlider->installEventFilter(tipFilter);

    // 3. 小标题 "标准色系" + 12 个色块
    QLabel* lblStandard = new QLabel("标准色系", container);
    lblStandard->setObjectName("FilterColorSubTitle");
    groupLayout->addWidget(lblStandard);

    QWidget* stdColorWidget = new QWidget(container);
    FlowLayout* stdFlow = new FlowLayout(stdColorWidget, 0, 2, 2);
    stdFlow->setContentsMargins(8, 0, 8, 0);

    static const QStringList standardHexes = {
        "#E24B4A", "#EF9F27", "#FECF0E", "#639922", "#1D9E75", "#378ADD",
        "#7F77DD", "#E91E63", "#000000", "#808080", "#FFFFFF", "#795548"
    };

    auto handleColorBlockClick = [filterModel, colorWheel, requestRebuild](const QString& hex) {
        QString hexUpper = hex.toUpper();
        QColor col(hexUpper);
        FilterState st = filterModel->state();
        if (st.colors.contains(hexUpper)) {
            st.colors.removeAll(hexUpper);
        } else {
            st.colors = QStringList{ hexUpper };
            pushRecent(hexUpper);
        }

        if (col.isValid()) {
            int h = col.hsvHue();
            if (h >= 0) {
                st.colorHue = h;
            }
        }

        colorWheel->setValues(st.colorHue, st.colorPrecision);
        filterModel->setState(st);
        if (requestRebuild) requestRebuild();
    };

    for (const QString& hex : standardHexes) {
        ColorBlock* cb = new ColorBlock(QColor(hex), stdColorWidget);
        cb->setCount(-1);
        cb->setChecked(currentState.colors.contains(hex.toUpper()));
        QObject::connect(cb, &ColorBlock::clicked, stdColorWidget, [handleColorBlockClick, hex](const QColor&) {
            handleColorBlockClick(hex);
        });
        stdFlow->addWidget(cb);
    }
    groupLayout->addWidget(stdColorWidget);

    // 4. 小标题 "最近筛选" + 最近使用色块
    QStringList recentColors = AppConfig::instance().getValue("Filter/RecentColors").toStringList();
    if (!recentColors.isEmpty()) {
        QLabel* lblRecent = new QLabel("最近筛选", container);
        lblRecent->setObjectName("FilterColorSubTitleRecent");
        groupLayout->addWidget(lblRecent);

        QWidget* recentColorWidget = new QWidget(container);
        FlowLayout* recentFlow = new FlowLayout(recentColorWidget, 0, 2, 2);
        recentFlow->setContentsMargins(8, 0, 8, 0);

        for (const QString& hex : recentColors) {
            ColorBlock* cb = new ColorBlock(QColor(hex), recentColorWidget);
            cb->setCount(-1);
            cb->setChecked(currentState.colors.contains(hex.toUpper()));
            QObject::connect(cb, &ColorBlock::clicked, recentColorWidget, [handleColorBlockClick, hex](const QColor&) {
                handleColorBlockClick(hex);
            });
            recentFlow->addWidget(cb);
        }
        groupLayout->addWidget(recentColorWidget);
    }

    contentLayout->addWidget(container);
}

} // namespace QuarkMeta
