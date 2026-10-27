#include "HistoryDao.h"
#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDir>
#include <QFileInfo>

namespace QuarkMeta {

bool HistoryDao::initTable() {
    auto handle = DatabaseManager::instance().getDiskDbHandle();
    if (!handle.isValid()) return false;

    QSqlQuery q(handle.db);
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS file_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_path TEXT NOT NULL UNIQUE COLLATE NOCASE,
            accessed_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";
    if (!q.exec(sql)) {
        DatabaseManager::logSqlError("HistoryDao::initTable create table", handle.db, sql);
        return false;
    }

    QString idxSql = "CREATE INDEX IF NOT EXISTS idx_history_accessed_at ON file_history(accessed_at DESC);";
    if (!q.exec(idxSql)) {
        DatabaseManager::logSqlError("HistoryDao::initTable create index", handle.db, idxSql);
        return false;
    }

    return true;
}

bool HistoryDao::addOrUpdateHistory(const QString& rawPath) {
    if (rawPath.isEmpty()) return false;
    QString cleanPath = QDir::toNativeSeparators(QFileInfo(rawPath).absoluteFilePath());

    auto handle = DatabaseManager::instance().getDiskDbHandle();
    if (!handle.isValid()) return false;

    QSqlQuery q(handle.db);
    q.prepare(R"(
        INSERT INTO file_history (file_path, accessed_at)
        VALUES (:path, CURRENT_TIMESTAMP)
        ON CONFLICT(file_path) DO UPDATE SET accessed_at = CURRENT_TIMESTAMP;
    )");
    q.bindValue(":path", cleanPath);
    if (!q.exec()) {
        DatabaseManager::logSqlError("HistoryDao::addOrUpdateHistory upsert", handle.db, q.lastQuery());
        return false;
    }

    // 保持最多 100 条，淘汰最旧数据
    QSqlQuery cleanQ(handle.db);
    QString cleanSql = R"(
        DELETE FROM file_history
        WHERE id NOT IN (
            SELECT id FROM file_history ORDER BY accessed_at DESC LIMIT 100
        );
    )";
    cleanQ.exec(cleanSql);

    return true;
}

QList<HistoryRecord> HistoryDao::getAllHistory() {
    QList<HistoryRecord> list;
    auto handle = DatabaseManager::instance().getDiskDbHandle();
    if (!handle.isValid()) return list;

    QSqlQuery q(handle.db);
    q.prepare("SELECT id, file_path, accessed_at FROM file_history ORDER BY accessed_at DESC LIMIT 100;");
    if (q.exec()) {
        while (q.next()) {
            HistoryRecord rec;
            rec.id = q.value(0).toInt();
            rec.filePath = q.value(1).toString();
            rec.accessedAt = q.value(2).toDateTime();
            list.append(rec);
        }
    }
    return list;
}

bool HistoryDao::removeHistory(const QString& rawPath) {
    if (rawPath.isEmpty()) return false;
    QString cleanPath = QDir::toNativeSeparators(QFileInfo(rawPath).absoluteFilePath());

    auto handle = DatabaseManager::instance().getDiskDbHandle();
    if (!handle.isValid()) return false;

    QSqlQuery q(handle.db);
    q.prepare("DELETE FROM file_history WHERE file_path = :path COLLATE NOCASE;");
    q.bindValue(":path", cleanPath);
    return q.exec();
}

bool HistoryDao::clearAllHistory() {
    auto handle = DatabaseManager::instance().getDiskDbHandle();
    if (!handle.isValid()) return false;

    QSqlQuery q(handle.db);
    return q.exec("DELETE FROM file_history;");
}

bool HistoryDao::updateHistoryPaths(const QStringList& oldPaths, const QStringList& newPaths) {
    if (oldPaths.size() != newPaths.size()) return false;
    auto handle = DatabaseManager::instance().getDiskDbHandle();
    if (!handle.isValid()) return false;

    for (int i = 0; i < oldPaths.size(); ++i) {
        QString oldP = QDir::toNativeSeparators(QFileInfo(oldPaths[i]).absoluteFilePath());
        QString newP = QDir::toNativeSeparators(QFileInfo(newPaths[i]).absoluteFilePath());

        QSqlQuery q(handle.db);
        q.prepare("UPDATE OR IGNORE file_history SET file_path = :newP, accessed_at = CURRENT_TIMESTAMP WHERE file_path = :oldP COLLATE NOCASE;");
        q.bindValue(":newP", newP);
        q.bindValue(":oldP", oldP);
        q.exec();

        // 如果与现有项产生冲突被 IGNORE，清理旧项
        QSqlQuery delQ(handle.db);
        delQ.prepare("DELETE FROM file_history WHERE file_path = :oldP COLLATE NOCASE;");
        delQ.bindValue(":oldP", oldP);
        delQ.exec();
    }
    return true;
}

} // namespace QuarkMeta
