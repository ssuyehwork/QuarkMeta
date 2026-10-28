#pragma once

#include <QUndoCommand>
#include <QStringList>
#include <QMap>

namespace QuarkMeta {

/**
 * @brief 标记目的地分发迁移复合撤销命令
 */
class TransferToMarkedCommand : public QUndoCommand {
public:
    struct TargetResult {
        QString targetDir;
        QString createdCopyPath;
    };

    TransferToMarkedCommand(const QStringList& sourcePaths,
                            const QMap<QString, QVector<TargetResult>>& copyResults,
                            const QStringList& recycledSources,
                            QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    QStringList m_sourcePaths;
    QMap<QString, QVector<TargetResult>> m_copyResults;
    QStringList m_recycledSources;
    bool m_isFirstRun = true;
};

} // namespace QuarkMeta
