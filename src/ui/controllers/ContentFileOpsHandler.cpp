#include "ContentFileOpsHandler.h"
#include "../ContentPanel.h"
#include "../ToolTipOverlay.h"
#include "../BatchRenameDialog.h"
#include "../FileCollisionDialog.h"
#include "../../core/AppConfig.h"
#include "../../core/ClipboardService.h"
#include "../../core/NavigationHistoryService.h"
#include "../../core/LastOperationManager.h"
#include "../../util/DiskIoService.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QApplication>
#include <QPointer>

namespace QuarkMeta {

ContentFileOpsHandler::ContentFileOpsHandler(ContentPanel* panel)
    : QObject(panel), m_panel(panel) {}

void ContentFileOpsHandler::createNewItem(const QString& type) {
    if (!m_panel) return;
    QString currentPath = m_panel->currentPath();
    if (currentPath.isEmpty() || currentPath == "computer://") return;

    QString baseName = (type == "folder") ? "新建文件夹" : "未命名";
    QString ext = (type == "md") ? ".md" : ((type == "txt") ? ".txt" : "");
    QString finalName = baseName + ext;
    QString fullPath = currentPath + "/" + finalName;
    int counter = 1;

    while (QFileInfo::exists(fullPath)) {
        finalName = baseName + QString(" (%1)").arg(counter++) + ext;
        fullPath = currentPath + "/" + finalName;
    }

    if (type == "folder") {
        QDir(currentPath).mkdir(finalName);
    } else {
        QFile f(fullPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.close();
        }
    }

    m_panel->setPendingSelectName(finalName, true);
    m_panel->loadDirectory(currentPath, m_panel->isRecursive());
}

void ContentFileOpsHandler::performBatchRename() {
    if (!m_panel) return;
    std::vector<std::wstring> originalPaths;
    for (const auto& idx : m_panel->getSelectedIndexes()) {
        if (idx.column() == 0) {
            QString p = idx.data(PathRole).toString();
            if (!p.isEmpty()) {
                originalPaths.push_back(QDir::toNativeSeparators(p).toStdWString());
            }
        }
    }
    if (originalPaths.empty()) return;

    BatchRenameDialog dlg(originalPaths, m_panel);
    if (dlg.exec() == QDialog::Accepted) {
        m_panel->refreshAll();
    }
}

bool ContentFileOpsHandler::resolvePasteDestination() {
    if (!m_panel) return false;
    if (m_panel->getCurrentCategoryType() == "trash") {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "当前视图为回收站，不支持粘贴或拖拽导入新项目", 2000, QColor("#e81123"));
        return false;
    }
    QString currentPath = m_panel->currentPath();
    if (currentPath.isEmpty() || currentPath == "computer://") {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "粘贴失败：当前未处于任何有效目录中", 2000, QColor("#e81123"));
        return false;
    }
    return true;
}

void ContentFileOpsHandler::onPathsDropped(const QStringList& paths, const QModelIndex& targetIndex) {
    if (!m_panel || paths.isEmpty()) return;
    QString currentPath = m_panel->currentPath();
    if (currentPath.isEmpty() || currentPath == "computer://") return;

    QString destDir = currentPath;
    if (targetIndex.isValid() && m_panel->getProxyModel()) {
        QModelIndex srcIdx = m_panel->getProxyModel()->mapToSource(targetIndex);
        if (srcIdx.isValid() && QFileInfo(srcIdx.data(PathRole).toString()).isDir()) {
            destDir = srcIdx.data(PathRole).toString();
        }
    }

    if (destDir.isEmpty() || destDir == "computer://") return;

    bool isMove = !(QApplication::keyboardModifiers() & Qt::ControlModifier);
    QString cleanDestDir = QDir::cleanPath(destDir);

    // 区分同目录拖拽与跨目录拖拽
    QStringList activePaths;
    DiskIoContext ioCtx;
    ioCtx.destination = destDir;
    ioCtx.isMove = isMove;

    bool recordMove = false;

    for (const QString& src : paths) {
        QString srcDir = QDir::cleanPath(QFileInfo(src).absolutePath());
        if (srcDir == cleanDestDir) {
            if (isMove) {
                // 1. 同目录下普通拖拽移动：自身移入所在目录无意义，直接跳过 (no-op)
                continue;
            } else {
                // 2. 同目录下 Ctrl+拖拽：执行“复制副本”操作，自动生成 (1) 副本
                activePaths.append(src);
                ioCtx.autoRenameFiles.insert(src);
            }
        } else {
            // 3. 跨目录拖拽移动 / 复制
            activePaths.append(src);
            if (isMove) {
                recordMove = true;
            }
        }
    }

    if (activePaths.isEmpty()) return;

    ioCtx.sources = activePaths;

    if (recordMove) {
        NavigationHistoryService::recordRecentVisitedFolder(QDir::toNativeSeparators(destDir).toStdWString());
        AppConfig::instance().setValue("RecentVisited/LastDragDropDestination", destDir);
        AppConfig::instance().sync();
        LastOperationManager::instance().recordMoveToFolder(destDir);
    }

    // 检测目标文件夹中的同名冲突文件（排除已设为自动复制副本的同目录复制项目）
    QStringList conflictingSources;
    for (const QString& src : activePaths) {
        if (ioCtx.autoRenameFiles.contains(src)) continue;

        QString fileName = QFileInfo(src).fileName();
        QString destPath = QDir(destDir).filePath(fileName);
        if (QFile::exists(destPath)) {
            conflictingSources.append(src);
        }
    }

    if (!conflictingSources.isEmpty()) {
        int remainingConflicts = conflictingSources.size();

        for (int i = 0; i < conflictingSources.size(); ++i) {
            const QString& srcFile = conflictingSources.at(i);
            FileCollisionDialog dialog(srcFile, destDir, remainingConflicts--, m_panel);

            if (dialog.exec() != QDialog::Accepted) {
                return; // 用户取消
            }

            CollisionResolveAction action = dialog.selectedAction();
            bool applyToAll = dialog.applyToAll();

            if (action == CollisionResolveAction::Cancel) {
                return;
            }

            if (applyToAll) {
                if (action == CollisionResolveAction::AutoResolve) {
                    ioCtx.autoRenameAll = true;
                } else if (action == CollisionResolveAction::Replace) {
                    ioCtx.overwriteAll = true;
                } else if (action == CollisionResolveAction::Skip) {
                    for (int j = i; j < conflictingSources.size(); ++j) {
                        ioCtx.sources.removeOne(conflictingSources.at(j));
                    }
                }
                break;
            } else {
                if (action == CollisionResolveAction::AutoResolve) {
                    ioCtx.autoRenameFiles.insert(srcFile);
                } else if (action == CollisionResolveAction::Replace) {
                    ioCtx.overwriteFiles.insert(srcFile);
                } else if (action == CollisionResolveAction::Skip) {
                    ioCtx.sources.removeOne(srcFile);
                }
            }
        }

        if (ioCtx.sources.isEmpty()) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), "已跳过所有同名文件", 1500, QColor("#378ADD"));
            return;
        }
    }

    QPointer<ContentPanel> weakPanel(m_panel);
    DiskIoService::instance().executeAsync(ioCtx, [weakPanel](bool success) {
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, success]() {
            if (weakPanel && success) {
                weakPanel->loadDirectory(weakPanel->currentPath(), weakPanel->isRecursive());
            }
        });
    });
}

} // namespace QuarkMeta
