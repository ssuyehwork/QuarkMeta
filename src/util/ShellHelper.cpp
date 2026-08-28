#include "ShellHelper.h"
#include <QFileInfo>
#include <QDateTime>
#include <atomic>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QCoreApplication>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

#include "../meta/MetadataManager.h"
#include "../meta/StatisticsService.h"
#include "../meta/QuarkMetaJson.h"
#include "../meta/FileOperationHelper.h"
#include "DiskMediaExtractor.h"

namespace QuarkMeta {

void ShellHelper::openInExplorer(const QString& path) {
    if (path.isEmpty() || path == "computer://" || path.contains("://")) return;

#ifdef Q_OS_WIN
    QStringList args;
    args << "/select," << QDir::toNativeSeparators(path);
    QProcess::startDetached("explorer", args);
#else
    Q_UNUSED(path);
#endif
}

void ShellHelper::showProperties(const QString& path) {
    if (path.isEmpty() || path.contains("://")) return;

#ifdef Q_OS_WIN
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_INVOKEIDLIST;
    sei.lpVerb = L"properties";
    std::wstring wpath = QDir::toNativeSeparators(path).toStdWString();
    sei.lpFile = wpath.c_str();
    sei.nShow = SW_SHOW;
    ShellExecuteExW(&sei);
#else
    Q_UNUSED(path);
#endif
}

bool ShellHelper::renameItem(const QString& oldPath, const QString& newPath) {
    if (oldPath.isEmpty() || newPath.isEmpty() || oldPath == newPath) return true;

    // 🚀【核心升级】：使用两阶段 UUID safeRename，彻底解决 Windows 大小写重命名失败
    if (FileOperationHelper::safeRename(oldPath, newPath)) {
        // 1. 物理漫游迁移本地 .QuarkMeta.json 元数据
        QuarkMetaJson::migrateItemMetadata(oldPath, newPath);

        // 2. 物理漫游磁盘 Hash 缩略图缓存
        QString oldThumbHashPath = DiskMediaExtractor::getDiskThumbCachePath(oldPath);
        QString newThumbHashPath = DiskMediaExtractor::getDiskThumbCachePath(newPath);
        if (QFile::exists(oldThumbHashPath)) {
            FileOperationHelper::safeRename(oldThumbHashPath, newThumbHashPath);
        }

        // 3. 同步更新 MetadataManager 内存缓存与 SQLite 索引
        MetadataManager::instance().renameItem(oldPath.toStdWString(), newPath.toStdWString());
        return true;
    }
    return false;
}

QString ShellHelper::formatSize(qint64 bytes) {
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 2);
    if (bytes < 1024LL * 1024 * 1024) return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 2);
    return QString("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}

void ShellHelper::ensureHidden(const std::wstring& path) {
#ifdef Q_OS_WIN
    SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN);
#else
    Q_UNUSED(path);
#endif
}

} // namespace QuarkMeta
