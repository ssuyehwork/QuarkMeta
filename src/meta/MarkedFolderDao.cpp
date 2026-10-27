#include "MarkedFolderDao.h"
#include "DatabaseManager.h"
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QDir>
#include <QMutexLocker>

namespace QuarkMeta {

bool MarkedFolderDao::initTable() {
    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlQuery query(DatabaseManager::instance().getDatabase());
    QString sql = R"(
        CREATE TABLE IF NOT EXISTS marked_folders (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            folder_path TEXT UNIQUE NOT NULL,
            is_marked INTEGER NOT NULL DEFAULT 1,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )";
    return query.exec(sql);
}

int MarkedFolderDao::getMarkedCount() {
    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlQuery query("SELECT COUNT(*) FROM marked_folders;", DatabaseManager::instance().getDatabase());
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

bool MarkedFolderDao::addMarkedFolder(const QString& folderPath) {
    if (folderPath.isEmpty()) return false;
    QString cleanPath = QDir::cleanPath(folderPath);

    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlDatabase db = DatabaseManager::instance().getDatabase();

    // 检查路径是否已在表中
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT id, is_marked FROM marked_folders WHERE folder_path = :path LIMIT 1;");
    checkQuery.bindValue(":path", cleanPath);
    if (checkQuery.exec() && checkQuery.next()) {
        // 已在表中，激活标记状态并更新时间
        QSqlQuery updateQuery(db);
        updateQuery.prepare("UPDATE marked_folders SET is_marked = 1, updated_at = CURRENT_TIMESTAMP WHERE folder_path = :path;");
        updateQuery.bindValue(":path", cleanPath);
        return updateQuery.exec();
    }

    // 新添加记录，检查数量上限（50条），自动执行淘汰规则
    QSqlQuery countQuery("SELECT COUNT(*) FROM marked_folders;", db);
    int totalCount = 0;
    if (countQuery.next()) {
        totalCount = countQuery.value(0).toInt();
    }

    if (totalCount >= 50) {
        // 步骤 1：优先淘汰 is_marked = 0 的最旧记录
        QSqlQuery evictUnmarkedQuery(db);
        evictUnmarkedQuery.prepare("DELETE FROM marked_folders WHERE id = (SELECT id FROM marked_folders WHERE is_marked = 0 ORDER BY updated_at ASC LIMIT 1);");
        if (evictUnmarkedQuery.exec() && evictUnmarkedQuery.numRowsAffected() > 0) {
            // 成功淘汰一条未标记记录
        } else {
            // 步骤 2：若无未标记记录可淘汰，淘汰最旧的标记记录
            QSqlQuery evictOldestQuery(db);
            evictOldestQuery.prepare("DELETE FROM marked_folders WHERE id = (SELECT id FROM marked_folders ORDER BY updated_at ASC LIMIT 1);");
            evictOldestQuery.exec();
        }
    }

    QSqlQuery insertQuery(db);
    insertQuery.prepare("INSERT INTO marked_folders (folder_path, is_marked, updated_at) VALUES (:path, 1, CURRENT_TIMESTAMP);");
    insertQuery.bindValue(":path", cleanPath);
    return insertQuery.exec();
}

bool MarkedFolderDao::unmarkFolder(const QString& folderPath) {
    if (folderPath.isEmpty()) return false;
    QString cleanPath = QDir::cleanPath(folderPath);

    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlQuery query(DatabaseManager::instance().getDatabase());
    query.prepare("UPDATE marked_folders SET is_marked = 0, updated_at = CURRENT_TIMESTAMP WHERE folder_path = :path;");
    query.bindValue(":path", cleanPath);
    return query.exec();
}

bool MarkedFolderDao::unmarkAllFolders() {
    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlQuery query(DatabaseManager::instance().getDatabase());
    return query.exec("UPDATE marked_folders SET is_marked = 0, updated_at = CURRENT_TIMESTAMP;");
}

bool MarkedFolderDao::removeFolderRecord(const QString& folderPath) {
    if (folderPath.isEmpty()) return false;
    QString cleanPath = QDir::cleanPath(folderPath);

    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlQuery query(DatabaseManager::instance().getDatabase());
    query.prepare("DELETE FROM marked_folders WHERE folder_path = :path;");
    query.bindValue(":path", cleanPath);
    return query.exec();
}

bool MarkedFolderDao::clearAllFolderRecords() {
    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlQuery query(DatabaseManager::instance().getDatabase());
    return query.exec("DELETE FROM marked_folders;");
}

bool MarkedFolderDao::isMarked(const QString& folderPath) {
    if (folderPath.isEmpty()) return false;
    QString cleanPath = QDir::cleanPath(folderPath);

    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlQuery query(DatabaseManager::instance().getDatabase());
    query.prepare("SELECT is_marked FROM marked_folders WHERE folder_path = :path LIMIT 1;");
    query.bindValue(":path", cleanPath);
    if (query.exec() && query.next()) {
        return query.value(0).toInt() == 1;
    }
    return false;
}

QStringList MarkedFolderDao::getAllMarkedFolders() {
    QStringList result;
    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlQuery query("SELECT folder_path FROM marked_folders ORDER BY is_marked DESC, updated_at DESC;", DatabaseManager::instance().getDatabase());
    while (query.next()) {
        result.append(query.value(0).toString());
    }
    return result;
}

QStringList MarkedFolderDao::getActiveMarkedFolders() {
    QStringList result;
    QMutexLocker locker(&DatabaseManager::getGlobalMutex());
    QSqlQuery query("SELECT folder_path FROM marked_folders WHERE is_marked = 1 ORDER BY updated_at DESC;", DatabaseManager::instance().getDatabase());
    while (query.next()) {
        result.append(query.value(0).toString());
    }
    return result;
}

} // namespace QuarkMeta
