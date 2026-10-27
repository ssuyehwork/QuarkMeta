#include "MarkedFolderService.h"
#include "MarkedFolderDao.h"
#include "../ui/UiHelper.h"
#include "../ui/ToolTipOverlay.h"
#include "../ui/dialogs/FramelessMessageBox.h"
#include "../core/TaskProgressService.h"
#include "../core/CentralEventHub.h"
#include "../core/DiskTrashService.h"
#include "../core/UndoManager.h"
#include "../util/ShellHelper.h"
#include <QFileInfo>
#include <QDir>
#include <QAction>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace QuarkMeta {

void TransferToMarkedCommand::undo() {
    int jobId = TaskProgressService::instance().createJob("撤销标记迁移", m_pairs.size());

    for (const auto& pair : m_pairs) {
        if (!pair.copiedPath.isEmpty() && QFile::exists(pair.copiedPath)) {
            DiskTrashService::moveToDiskTrash({pair.copiedPath});
        }
        if (!pair.trashItemPath.isEmpty()) {
            DiskTrashService::restoreFromDiskTrash(0, pair.trashItemPath);
        }
    }

    TaskProgressService::instance().finishJob(jobId);
}

void TransferToMarkedCommand::redo() {
    int jobId = TaskProgressService::instance().createJob("重做标记迁移", m_pairs.size());

    for (const auto& pair : m_pairs) {
        if (!pair.originalPath.isEmpty() && QFile::exists(pair.originalPath) && !pair.copiedPath.isEmpty()) {
            ShellHelper::copyOrMoveItems({pair.originalPath}, QFileInfo(pair.copiedPath).absolutePath(), false, false, false, {}, {});
        }
        if (!pair.trashItemPath.isEmpty() && !pair.originalPath.isEmpty() && QFile::exists(pair.originalPath)) {
            DiskTrashService::moveToDiskTrash({pair.originalPath});
        }
    }

    TaskProgressService::instance().finishJob(jobId);
}

MarkedFolderService& MarkedFolderService::instance() {
    static MarkedFolderService s_instance;
    return s_instance;
}

MarkedFolderService::MarkedFolderService(QObject* parent) : QObject(parent) {
    MarkedFolderDao::initTable();

    connect(&CentralEventHub::instance(), &CentralEventHub::eventOccurred, this, [this](const QuarkMeta::AppEvent& evt) {
        if (evt.type == AppEventType::ItemsDeleted || evt.type == AppEventType::ItemsRenamed) {
            for (const QString& path : evt.paths) {
                if (!QFile::exists(path) && isMarked(path)) {
                    MarkedFolderDao::unmarkFolder(path);
                    emit markedFoldersChanged();
                }
            }
        }
    });
}

static bool isSameVolume(const QString& pathA, const QString& pathB) {
#ifdef Q_OS_WIN
    wchar_t volA[MAX_PATH] = {0};
    wchar_t volB[MAX_PATH] = {0};
    std::wstring wA = QDir::toNativeSeparators(pathA).toStdWString();
    std::wstring wB = QDir::toNativeSeparators(pathB).toStdWString();

    if (GetVolumePathNameW(wA.c_str(), volA, MAX_PATH) &&
        GetVolumePathNameW(wB.c_str(), volB, MAX_PATH)) {
        return (QString::fromWCharArray(volA).compare(QString::fromWCharArray(volB), Qt::CaseInsensitive) == 0);
    }
#endif
    return (QFileInfo(pathA).absoluteFilePath().left(3).compare(QFileInfo(pathB).absoluteFilePath().left(3), Qt::CaseInsensitive) == 0);
}

void MarkedFolderService::executeTransferToMarked(const QStringList& sourcePaths) {
    QStringList rawTargets = getActiveMarked();
    if (sourcePaths.isEmpty() || rawTargets.isEmpty()) {
        ToolTipOverlay::instance()->showToast("未选中项目或未激活标记目的地", 1500, QColor("#e81123"));
        return;
    }

    // 1. 过滤非法目标：排除选中项目自身、其子目录、以及包含选中的直接父目录
    QStringList validTargets;
    for (const QString& dst : rawTargets) {
        bool isValid = true;
        for (const QString& src : sourcePaths) {
            QString srcParent = QFileInfo(src).absolutePath();
            if (dst == src || dst.startsWith(src + "/") || dst == srcParent) {
                isValid = false;
                break;
            }
        }
        if (isValid) validTargets.append(dst);
    }

    if (validTargets.isEmpty()) {
        ToolTipOverlay::instance()->showToast("目标文件夹不能是选中项目自身、其子目录或当前所在目录", 1500, QColor("#e81123"));
        return;
    }

    // 2. 异盘/同盘模式计算
    bool hasSameVol = false;
    for (const QString& src : sourcePaths) {
        for (const QString& dst : validTargets) {
            if (isSameVolume(src, dst)) {
                hasSameVol = true;
                break;
            }
        }
    }

    // 3. 弹窗确认
    QString msg = QString("即将将 %1 个选中的项目迁移到 %2 个已标记的目的地文件夹中。\n模式: %3\n确定要继续吗？")
                      .arg(sourcePaths.size())
                      .arg(validTargets.size())
                      .arg(hasSameVol ? "迁移 (同盘复制并回收原文件)" : "复制 (异盘纯复制)");

    if (!FramelessMessageBox::question(nullptr, "确认迁移", msg)) {
        return;
    }

    // 4. 创建进度条并执行复制/回收与撤销记单
    int jobId = TaskProgressService::instance().createJob("迁移至标记目的地", sourcePaths.size() * validTargets.size());

    QVector<TransferPair> pairs;
    bool allSuccess = true;

    for (const QString& dst : validTargets) {
        if (!ShellHelper::copyOrMoveItems(sourcePaths, dst, false, false, false, {}, {})) {
            allSuccess = false;
            break;
        }
        for (const QString& src : sourcePaths) {
            TransferPair pair;
            pair.originalPath = src;
            pair.copiedPath = QDir::cleanPath(dst + "/" + QFileInfo(src).fileName());
            pairs.append(pair);
        }
    }

    if (allSuccess) {
        if (hasSameVol) {
            DiskTrashService::moveToDiskTrash(sourcePaths);
            for (int i = 0; i < pairs.size(); ++i) {
                pairs[i].trashItemPath = pairs[i].originalPath;
            }
        }
        UndoManager::instance().pushCommand(std::make_unique<TransferToMarkedCommand>(pairs));
    }

    TaskProgressService::instance().finishJob(jobId);
}

bool MarkedFolderService::isMarked(const QString& path) const {
    if (path.isEmpty()) return false;
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isDir()) return false;
    return MarkedFolderDao::isMarked(path);
}

int MarkedFolderService::markedCount() const {
    return MarkedFolderDao::getMarkedCount();
}

bool MarkedFolderService::addMarked(const QString& path) {
    if (path.isEmpty()) return false;
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isDir()) return false;

    if (MarkedFolderDao::addMarkedFolder(path)) {
        emit markedFoldersChanged();
        return true;
    }
    return false;
}

bool MarkedFolderService::removeMarked(const QString& path) {
    if (MarkedFolderDao::unmarkFolder(path)) {
        emit markedFoldersChanged();
        return true;
    }
    return false;
}

bool MarkedFolderService::toggleMarked(const QString& path) {
    if (isMarked(path)) {
        return removeMarked(path);
    } else {
        return addMarked(path);
    }
}

QStringList MarkedFolderService::getAllMarked() const {
    return MarkedFolderDao::getAllMarkedFolders();
}

QStringList MarkedFolderService::getActiveMarked() const {
    return MarkedFolderDao::getActiveMarkedFolders();
}

bool MarkedFolderService::removeFolderRecord(const QString& path) {
    if (MarkedFolderDao::removeFolderRecord(path)) {
        emit markedFoldersChanged();
        return true;
    }
    return false;
}

bool MarkedFolderService::clearAllFolderRecords() {
    if (MarkedFolderDao::clearAllFolderRecords()) {
        emit markedFoldersChanged();
        return true;
    }
    return false;
}

bool MarkedFolderService::clearAll() {
    if (MarkedFolderDao::unmarkAllFolders()) {
        emit markedFoldersChanged();
        return true;
    }
    return false;
}

QAction* MarkedFolderService::buildMarkAction(QMenu* parentMenu, const QString& path, QObject* receiver) {
    if (!parentMenu) return nullptr;
    QFileInfo fi(path);
    if (!fi.exists() || !fi.isDir()) return nullptr;

    bool marked = isMarked(path);
    QString label = marked ? "取消标记" : "标记为目的地";
    QIcon icon = UiHelper::getIcon("Destination.svg", QColor("#ff551c"), 18);

    QAction* action = parentMenu->addAction(icon, label);
    if (receiver) {
        connect(action, &QAction::triggered, receiver, [this, path]() {
            toggleMarked(path);
        });
    }
    return action;
}

} // namespace QuarkMeta
