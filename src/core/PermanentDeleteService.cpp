#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "PermanentDeleteService.h"
#include "../ui/dialogs/FramelessMessageBox.h"
#include "../ui/BatchProgressDialog.h"
#include "../ui/ToolTipOverlay.h"
#include "../util/SecureFileEraser.h"
#include "../core/UndoManager.h"
#include "../core/CoreEngine.h"
#include "../core/DiskTrashService.h"
#include "../meta/MetadataManager.h"
#include "../meta/TrashRepository.h"
#include "../meta/QuarkMetaJson.h"
#include "../meta/DiskTrashRepo.h"
#include <QtConcurrent>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QPointer>
#include <QCoreApplication>
#include <QCursor>

namespace QuarkMeta {

PermanentDeleteService& PermanentDeleteService::instance() {
    static PermanentDeleteService s_instance;
    return s_instance;
}

PermanentDeleteService::PermanentDeleteService(QObject* parent) : QObject(parent) {}

bool PermanentDeleteService::execute(const QStringList& paths, QWidget* parentWidget, bool isSecureShred) {
    if (paths.isEmpty()) return false;

    QString msg = "确定要永久删除选中的项目吗？数据将被物理覆写并彻底抹除，此操作不可恢复。";
    if (!FramelessMessageBox::question(parentWidget, "确认删除", msg)) {
        return false;
    }

    QPointer<BatchProgressDialog> dlg;
    QMetaObject::invokeMethod(QCoreApplication::instance(), [&dlg]() {
        dlg = new BatchProgressDialog("永久删除");
        dlg->show();
    }, Qt::BlockingQueuedConnection);

    QPointer<PermanentDeleteService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([paths, isSecureShred, dlg, weakThis]() {
        int total = paths.size();
        int count = 0;

        for (const QString& p : paths) {
            std::wstring wp = QDir::toNativeSeparators(p).toStdWString();
            
            int trashId = -1;
            QString trashPath;
            bool foundRecord = TrashRepository::instance().getDiskTrashRecordByPath(wp, trashId, trashPath);
            bool isTrashItem = foundRecord || p.contains(".QuarkMeta/disk_trash");

            if (isTrashItem) {
                if (isSecureShred) {
                    SecureFileEraser::shredFile(p);
                }
                DiskTrashService::permanentlyDeleteDiskTrash(trashId, p);
                UndoManager::instance().removeCommandsForPath(p);
            } else {
                bool physicalOk = false;
                if (isSecureShred) {
                    physicalOk = SecureFileEraser::shredFile(p);
                } else {
                    QFileInfo info(p);
                    if (info.isDir()) {
                        physicalOk = QDir(p).removeRecursively();
                    } else {
                        physicalOk = QFile::remove(p);
                    }
                }

                if (physicalOk) {
                    QuarkMetaJson::removeItemMetadata(p);
                    AppCommand cmd;
                    cmd.type = AppCommandType::DeletePermanently;
                    cmd.targetPaths << QString::fromStdWString(wp);
                    CoreEngine::instance().executeCommand(cmd);
                    UndoManager::instance().removeCommandsForPath(p);
                }
            }

            count++;
            if (dlg) {
                QMetaObject::invokeMethod(dlg.data(), "updateProgress", Qt::QueuedConnection,
                                          Q_ARG(int, count), Q_ARG(int, total), Q_ARG(QString, QFileInfo(p).fileName()));
            }
        }

        if (dlg) {
            QMetaObject::invokeMethod(dlg.data(), "close", Qt::QueuedConnection);
            QMetaObject::invokeMethod(dlg.data(), "deleteLater", Qt::QueuedConnection);
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis]() {
            MetadataManager::instance().endInternalOperation();
            if (weakThis) {
                emit weakThis->permanentDeleteCompleted();
            }
        });
    });

    return true;
}

bool PermanentDeleteService::executeTrashItems(const std::vector<std::pair<int, QString>>& trashItems, QWidget* parentWidget) {
    if (trashItems.empty()) return false;

    QString msg = "确定要永久删除选中的回收站项目吗？数据将被彻底抹除且不可恢复。";
    if (!FramelessMessageBox::question(parentWidget, "确认删除", msg)) {
        return false;
    }

    QPointer<BatchProgressDialog> dlg;
    QMetaObject::invokeMethod(QCoreApplication::instance(), [&dlg]() {
        dlg = new BatchProgressDialog("销毁回收站项目");
        dlg->show();
    }, Qt::BlockingQueuedConnection);

    QPointer<PermanentDeleteService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([trashItems, dlg, weakThis]() {
        int total = static_cast<int>(trashItems.size());
        int count = 0;

        for (const auto& item : trashItems) {
            SecureFileEraser::shredFile(item.second);
            DiskTrashService::permanentlyDeleteDiskTrash(item.first, item.second);
            UndoManager::instance().removeCommandsForPath(item.second);
            count++;
            if (dlg) {
                QMetaObject::invokeMethod(dlg.data(), "updateProgress", Qt::QueuedConnection,
                                          Q_ARG(int, count), Q_ARG(int, total), Q_ARG(QString, QFileInfo(item.second).fileName()));
            }
        }

        if (dlg) {
            QMetaObject::invokeMethod(dlg.data(), "close", Qt::QueuedConnection);
            QMetaObject::invokeMethod(dlg.data(), "deleteLater", Qt::QueuedConnection);
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis]() {
            MetadataManager::instance().endInternalOperation();
            if (weakThis) {
                emit weakThis->permanentDeleteCompleted();
            }
        });
    });

    return true;
}

bool PermanentDeleteService::executeEmptyTrash(QWidget* parentWidget, bool isSecureShred) {
    auto allTrash = DiskTrashRepo::getAllTrashItems();
    if (allTrash.empty()) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "回收站为空，无需清理", 1500);
        return true;
    }

    QString msg = QString("确定要清空回收站吗？共有 %1 个项目将被物理覆写并彻底抹除，此操作不可恢复。").arg(allTrash.size());
    if (!FramelessMessageBox::question(parentWidget, "清空回收站确认", msg)) {
        return false;
    }

    QPointer<BatchProgressDialog> dlg;
    QMetaObject::invokeMethod(QCoreApplication::instance(), [&dlg]() {
        dlg = new BatchProgressDialog("清空回收站");
        dlg->show();
    }, Qt::BlockingQueuedConnection);

    QPointer<PermanentDeleteService> weakThis(this);

    MetadataManager::instance().beginInternalOperation();

    (void)QtConcurrent::run([allTrash, isSecureShred, dlg, weakThis]() {
        int total = static_cast<int>(allTrash.size());
        int count = 0;

        for (const auto& item : allTrash) {
            QString path = QString::fromStdWString(item.trashPath);
            if (isSecureShred) {
                SecureFileEraser::shredFile(path);
            }
            DiskTrashService::permanentlyDeleteDiskTrash(item.id, path);
            UndoManager::instance().removeCommandsForPath(QString::fromStdWString(item.originalPath));

            count++;
            if (dlg) {
                QMetaObject::invokeMethod(dlg.data(), "updateProgress", Qt::QueuedConnection,
                                          Q_ARG(int, count), Q_ARG(int, total), Q_ARG(QString, QFileInfo(path).fileName()));
            }
        }

        if (dlg) {
            QMetaObject::invokeMethod(dlg.data(), "close", Qt::QueuedConnection);
            QMetaObject::invokeMethod(dlg.data(), "deleteLater", Qt::QueuedConnection);
        }

        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakThis]() {
            MetadataManager::instance().endInternalOperation();
            MetadataManager::instance().notifyUI(MetadataManager::RefreshLevel::FullRebuild);
            if (weakThis) {
                emit weakThis->permanentDeleteCompleted();
            }
        });
    });

    return true;
}

} // namespace QuarkMeta