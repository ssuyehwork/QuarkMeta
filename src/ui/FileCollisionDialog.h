#pragma once

#include "FramelessDialog.h"
#include <QCheckBox>
#include <QPushButton>

namespace QuarkMeta {

enum class CollisionResolveAction {
    Replace,  // 替换目标中的文件
    Skip,     // 跳过这些文件
    KeepBoth, // 保留两者（自动重命名）
    Cancel    // 取消操作
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
