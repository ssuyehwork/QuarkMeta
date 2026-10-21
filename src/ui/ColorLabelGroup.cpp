#include "ColorLabelGroup.h"
#include "StyleLibrary.h"

namespace QuarkMeta {

void ColorLabelGroup::populate(QWidget* parentWidget,
                               QVBoxLayout* contentLayout,
                               FilterStateModel* filterModel,
                               const QMap<QString, int>& colorCounts,
                               const FilterState& currentState,
                               AddFilterRowFunc addFilterRow) {
    if (!contentLayout || !filterModel || !addFilterRow) return;

    QWidget* contextObj = parentWidget ? parentWidget : contentLayout->parentWidget();
    const auto& colorsList = Style::getColorPalette();

    for (const auto& item : colorsList) {
        QString itemNormHex = Style::normalizeManualColor(item.hex.isEmpty() ? item.name : item.hex);
        QString rowKey = QString("color:%1").arg(itemNormHex);

        int cnt = colorCounts.value(itemNormHex, colorCounts.value(item.hex, colorCounts.value(item.name, 0)));

        bool isChecked = false;
        for (const QString& mc : currentState.manualColors) {
            if (Style::normalizeManualColor(mc) == itemNormHex) {
                isChecked = true;
                break;
            }
        }

        if (cnt == 0 && !isChecked) {
            continue;
        }

        QCheckBox* cb = addFilterRow(contentLayout, item.name, cnt, item.color, rowKey);
        cb->setChecked(isChecked);
        QObject::connect(cb, &QCheckBox::checkStateChanged, contextObj, [filterModel, itemNormHex, name = item.name](Qt::CheckState state) {
            FilterState st = filterModel->state();
            if (state == Qt::Checked) {
                bool exists = false;
                for (const QString& mc : st.manualColors) {
                    if (Style::normalizeManualColor(mc) == itemNormHex) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    st.manualColors.append(name);
                }
            } else {
                QStringList filtered;
                for (const QString& mc : st.manualColors) {
                    if (Style::normalizeManualColor(mc) != itemNormHex) {
                        filtered.append(mc);
                    }
                }
                st.manualColors = filtered;
            }
            filterModel->setState(st);
        });
    }
}

} // namespace QuarkMeta
