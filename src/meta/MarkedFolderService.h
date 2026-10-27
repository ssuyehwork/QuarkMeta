#pragma once

#include <QObject>
#include <QStringList>
#include <QMenu>
#include <QVector>
#include "../core/UndoManager.h"
#include "../core/ActionCommand.h"

namespace QuarkMeta {

struct TransferPair {
    QString originalPath;
    QString copiedPath;
    QString trashItemPath;
};

class TransferToMarkedCommand : public ActionCommand {
public:
    explicit TransferToMarkedCommand(const QVector<TransferPair>& pairs) : m_pairs(pairs) {}
    void undo() override;
    void redo() override;
    bool affectsPath(const QString& path) const override {
        for (const auto& pair : m_pairs) {
            if (pair.originalPath == path || pair.copiedPath == path || pair.trashItemPath == path) return true;
        }
        return false;
    }
private:
    QVector<TransferPair> m_pairs;
};

class MarkedFolderService : public QObject {
    Q_OBJECT

public:
    static MarkedFolderService& instance();
    explicit MarkedFolderService(QObject* parent = nullptr);
    ~MarkedFolderService() override = default;

    bool isMarked(const QString& path) const;
    bool addMarked(const QString& path);
    bool removeMarked(const QString& path);
    bool toggleMarked(const QString& path);
    int markedCount() const;
    QStringList getAllMarked() const;
    QStringList getActiveMarked() const;
    bool removeFolderRecord(const QString& path);
    bool clearAllFolderRecords();
    bool clearAll();

    QAction* buildMarkAction(QMenu* parentMenu, const QString& path, QObject* receiver);
    void executeTransferToMarked(const QStringList& sourcePaths);

signals:
    void markedFoldersChanged();

private:
    Q_DISABLE_COPY_MOVE(MarkedFolderService)
};

} // namespace QuarkMeta
