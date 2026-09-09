#pragma once

#include "FramelessDialog.h"
#include <QCheckBox>
#include <QPushButton>

namespace QuarkMeta {

enum class CollisionResolveAction {
    AutoResolve, // 自动解析（同时共存 / 自动重命名）
    Replace,     // 替代（直接覆盖）
    Cancel       // 取消操作
};

class FileCollisionDialog : public FramelessDialog {
    Q_OBJECT
public:
    explicit FileCollisionDialog(const QString& sourceDir,
                                 const QString& targetDir,
                                 int conflictCount,
                                 QWidget* parent = nullptr);

    CollisionResolveAction selectedAction() const;
    bool applyToAll() const;

private:
    void setupUi(const QString& sourceDir, const QString& targetDir, int conflictCount);

    CollisionResolveAction m_selectedAction = CollisionResolveAction::Cancel;
    QCheckBox* m_chkApplyToAll = nullptr;
};

} // namespace QuarkMeta
