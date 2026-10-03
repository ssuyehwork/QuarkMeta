#include "AspectRatioGroup.h"
#include <QButtonGroup>
#include <QList>
#include <tuple>

namespace QuarkMeta {

void AspectRatioGroup::populate(QWidget* parentWidget,
                                QVBoxLayout* contentLayout,
                                FilterStateModel* filterModel,
                                const QuarkMeta::ScanStats& currentStats,
                                const FilterState& currentState,
                                AddFilterRowFunc addFilterRow) {
    if (!contentLayout || !filterModel || !addFilterRow) return;

    QButtonGroup* ratioGroup = new QButtonGroup(parentWidget ? parentWidget : contentLayout->parentWidget());
    ratioGroup->setExclusive(false);

    const QList<std::tuple<FilterState::AspectRatio, QString, int>> ratioItems = {
        {FilterState::Horizontal, "横图", currentStats.ratioHorizontalCount},
        {FilterState::Vertical, "竖图", currentStats.ratioVerticalCount},
        {FilterState::Square, "方形", currentStats.ratioSquareCount},
        {FilterState::Ratio169, "16:9", currentStats.ratio169Count}
    };
    for (const auto& [ratio, label, count] : ratioItems) {
        QCheckBox* cb = addFilterRow(contentLayout, label, count);
        if (currentState.ratio == ratio) cb->setChecked(true);
        QObject::connect(cb, &QCheckBox::toggled, cb, [filterModel, ratio, ratioGroup, cb](bool on) {
            FilterState st = filterModel->state();
            if (on) {
                for (QAbstractButton* b : ratioGroup->buttons()) if (b != cb && b->isChecked()) b->setChecked(false);
                st.ratio = ratio;
            } else st.ratio = FilterState::AspectAny;
            filterModel->setState(st);
        });
        ratioGroup->addButton(cb);
    }
}

} // namespace QuarkMeta
