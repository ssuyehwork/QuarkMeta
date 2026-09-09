#include "FileCollisionDialog.h"
#include "UiHelper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileInfo>
#include <QDir>

namespace QuarkMeta {

FileCollisionDialog::FileCollisionDialog(const QString& sourceDir,
                                           const QString& targetDir,
                                           int conflictCount,
                                           QWidget* parent)
    : FramelessDialog("替换或跳过文件", parent) {
    setFixedSize(460, 310);
    setupUi(sourceDir, targetDir, conflictCount);
}

CollisionResolveAction FileCollisionDialog::selectedAction() const {
    return m_selectedAction;
}

bool FileCollisionDialog::applyToAll() const {
    return m_chkApplyToAll ? m_chkApplyToAll->isChecked() : false;
}

void FileCollisionDialog::setupUi(const QString& sourceDir, const QString& targetDir, int conflictCount) {
    QWidget* area = getContentArea();
    QVBoxLayout* layout = new QVBoxLayout(area);
    layout->setContentsMargins(24, 10, 24, 20);
    layout->setSpacing(14);

    QString srcName = QFileInfo(sourceDir).fileName();
    if (srcName.isEmpty()) srcName = sourceDir;
    QString tgtName = QFileInfo(targetDir).fileName();
    if (tgtName.isEmpty()) tgtName = targetDir;

    QLabel* lblHeader = new QLabel(QString("正在将 %1 个项目从 <font color='#378ADD'>%2</font> 复制到 <font color='#378ADD'>%3</font>")
                                      .arg(conflictCount)
                                      .arg(srcName.toHtmlEscaped())
                                      .arg(tgtName.toHtmlEscaped()), area);
    lblHeader->setWordWrap(true);
    lblHeader->setStyleSheet("color: #CCCCCC; font-size: 13px;");
    layout->addWidget(lblHeader);

    QLabel* lblSub = new QLabel(QString("目标包含 %1 个同名文件").arg(conflictCount), area);
    lblSub->setStyleSheet("color: #FFFFFF; font-size: 16px; font-weight: bold;");
    layout->addWidget(lblSub);

    auto createOptionBtn = [this, area](const QString& iconName, const QString& text, CollisionResolveAction action) {
        QPushButton* btn = new QPushButton(area);
        btn->setIcon(UiHelper::getIcon(iconName, QColor("#EEEEEE"), 18));
        btn->setText("  " + text);
        btn->setFixedHeight(40);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QPushButton {"
            "  background-color: #2D2D30;"
            "  color: #FFFFFF;"
            "  border: 1px solid #3E3E42;"
            "  border-radius: 4px;"
            "  text-align: left;"
            "  padding-left: 16px;"
            "  font-size: 14px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #3E3E42;"
            "  border-color: #378ADD;"
            "}"
        );
        connect(btn, &QPushButton::clicked, this, [this, action]() {
            m_selectedAction = action;
            accept();
        });
        return btn;
    };

    layout->addWidget(createOptionBtn("copy", "自动解析（同时共存）", CollisionResolveAction::AutoResolve));
    layout->addWidget(createOptionBtn("paste", "替代", CollisionResolveAction::Replace));
    layout->addWidget(createOptionBtn("close", "取消", CollisionResolveAction::Cancel));

    m_chkApplyToAll = new QCheckBox("为所有冲突执行此操作", area);
    m_chkApplyToAll->setStyleSheet(
        "QCheckBox { color: #CCCCCC; font-size: 13px; }"
        "QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid #555555; border-radius: 2px; background: #2D2D30; }"
        "QCheckBox::indicator:checked { background: #378ADD; border-color: #378ADD; }"
    );
    layout->addWidget(m_chkApplyToAll);
}

} // namespace QuarkMeta
