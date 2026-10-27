#include "ContentKeyHandler.h"
#include "../ContentPanel.h"
#include "../CardLayoutEngine.h"
#include "../RatingBarLayout.h"
#include <memory>
#include <atomic>
#include "../ToolTipOverlay.h"
#include "../../meta/MarkedFolderService.h"
#include "../../core/DiskTrashService.h"
#include "../../util/TaskProgressService.h"
#include "../../core/UndoManager.h"
#include "../AppShortcutController.h"
#include "../dialogs/FramelessConfirmDialog.h"
#include <QStorageInfo>
#include "../ShellIconManager.h"
#include "../UiHelper.h"
#include "../FileCollisionDialog.h"
#include "../../core/TrashService.h"
#include "../../core/PermanentDeleteService.h"
#include "../../core/ClipboardService.h"
#include "../../core/AppConfig.h"
#include "../../core/ModelContract.h"
#include "../../util/DiskIoService.h"
#include "../../core/LastOperationManager.h"
#include "../../core/CoreEngine.h"
#include "ContextMenuFactory.h"
#include "../CardPainterHelper.h"
#include <QPointer>

#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QAbstractItemView>
#include <QTreeView>
#include <QLineEdit>
#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QSet>
#include <QTimer>

namespace QuarkMeta {

ContentKeyHandler::ContentKeyHandler(ContentPanel* panel, QObject* parent)
    : QObject(parent), m_panel(panel) {
}

bool ContentKeyHandler::executeRepeatLastOp(ContentPanel* panel, QAbstractItemView* view) {
    if (!LastOperationManager::instance().hasOperation()) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "尚未记录任何可重复的操作", 1500, QColor("#e81123"));
        return false;
    }

    LastOperationType type = LastOperationManager::instance().type();
    if (type == LastOperationType::MoveToFolder) {
        return executeMoveToFolder(panel, LastOperationManager::instance().destination());
    }

    if (!view) return false;
    QAbstractItemModel* model = view->model();
    if (!model) return false;

    auto indexes = view->selectionModel()->selectedIndexes();
    int count = 0;

    if (type == LastOperationType::SetRating) {
        QStringList selectedPaths;
        for (const auto& selIdx : indexes) {
            if (selIdx.column() == 0 && !selIdx.data(SectionHeaderRole).toBool()) {
                QString p = selIdx.data(PathRole).toString();
                if (!p.isEmpty()) selectedPaths.append(p);
            }
        }
        if (!selectedPaths.isEmpty()) {
            AppCommand cmd;
            cmd.type = AppCommandType::SetRating;
            cmd.targetPaths = selectedPaths;
            cmd.params["rating"] = LastOperationManager::instance().rating();
            CoreEngine::instance().executeCommand(cmd);
                    if (panel) {
                        for (const QString& p : selectedPaths) {
                            panel->updateItemMetadata(p);
                        }
                        panel->recalculateAndEmitStats();
            }
            count = selectedPaths.size();
        }
    } else if (type == LastOperationType::SetColor) {
        QStringList selectedPaths;
        for (const auto& selIdx : indexes) {
            if (selIdx.column() == 0 && !selIdx.data(SectionHeaderRole).toBool()) {
                QString p = selIdx.data(PathRole).toString();
                if (!p.isEmpty()) selectedPaths.append(p);
            }
        }
        if (!selectedPaths.isEmpty()) {
            AppCommand cmd;
            cmd.type = AppCommandType::SetColor;
            cmd.targetPaths = selectedPaths;
            cmd.params["color"] = LastOperationManager::instance().color();
            CoreEngine::instance().executeCommand(cmd);
                    if (panel) {
                        for (const QString& p : selectedPaths) {
                            panel->updateItemMetadata(p);
                        }
                        panel->recalculateAndEmitStats();
            }
            count = selectedPaths.size();
        }
    } else if (type == LastOperationType::PasteTags) {
        for (const auto& idx : indexes) {
            if (idx.column() == static_cast<int>(FileListColumn::Name) && !idx.data(SectionHeaderRole).toBool()) {
                model->setData(idx, LastOperationManager::instance().tags(), TagsRole);
                count++;
            }
        }
    }

    if (count > 0) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已对 %1 个项目重复执行上一次操作").arg(count), 1500, QColor("#2ecc71"));
    }
    return count > 0;
}

bool ContentKeyHandler::executeMoveToFolder(ContentPanel* panel, const QString& targetDir) {
    if (!panel) return false;
    if (targetDir.isEmpty() || !QDir(targetDir).exists()) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "目标文件夹不存在或尚未发生过移入操作", 1500, QColor("#e81123"));
        return false;
    }

    QStringList selectedPaths = panel->getSelectedPaths();
    if (selectedPaths.isEmpty()) {
        ToolTipOverlay::instance()->showText(QCursor::pos(), "未选择任何项目", 1200, QColor("#e81123"));
        return false;
    }

    // 检测目标文件夹中的同名冲突文件
    QStringList conflictingSources;
    for (const QString& src : selectedPaths) {
        QString fileName = QFileInfo(src).fileName();
        QString destPath = QDir(targetDir).filePath(fileName);
        if (QFile::exists(destPath)) {
            conflictingSources.append(src);
        }
    }

    DiskIoContext ioCtx;
    ioCtx.sources = selectedPaths;
    ioCtx.destination = targetDir;
    ioCtx.isMove = true;

    if (!conflictingSources.isEmpty()) {
        QStringList activeSources = selectedPaths;
        int remainingConflicts = conflictingSources.size();

        for (int i = 0; i < conflictingSources.size(); ++i) {
            const QString& srcFile = conflictingSources.at(i);
            FileCollisionDialog dialog(srcFile, targetDir, remainingConflicts--, panel);

            if (dialog.exec() != QDialog::Accepted) {
                return true;
            }

            CollisionResolveAction action = dialog.selectedAction();
            bool applyToAll = dialog.applyToAll();

            if (action == CollisionResolveAction::Cancel) {
                return true;
            }

            if (applyToAll) {
                if (action == CollisionResolveAction::AutoResolve) {
                    ioCtx.autoRenameAll = true;
                } else if (action == CollisionResolveAction::Replace) {
                    ioCtx.overwriteAll = true;
                } else if (action == CollisionResolveAction::Skip) {
                    for (int j = i; j < conflictingSources.size(); ++j) {
                        activeSources.removeOne(conflictingSources.at(j));
                    }
                }
                break;
            } else {
                if (action == CollisionResolveAction::AutoResolve) {
                    ioCtx.autoRenameFiles.insert(srcFile);
                } else if (action == CollisionResolveAction::Replace) {
                    ioCtx.overwriteFiles.insert(srcFile);
                } else if (action == CollisionResolveAction::Skip) {
                    activeSources.removeOne(srcFile);
                }
            }
        }

        if (activeSources.isEmpty()) {
            ToolTipOverlay::instance()->showText(QCursor::pos(), "已跳过所有同名文件", 1500, QColor("#378ADD"));
            return true;
        }
        ioCtx.sources = activeSources;
    }

    QPointer<ContentPanel> weakPanel(panel);
    DiskIoService::instance().executeAsync(ioCtx, [weakPanel, targetDir](bool success) {
        QMetaObject::invokeMethod(QCoreApplication::instance(), [weakPanel, targetDir, success]() {
            if (weakPanel) {
                if (success) {
                    weakPanel->refreshAll();
                    QString folderName = QFileInfo(targetDir).fileName();
                    ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已移入到: %1").arg(folderName), 1500, QColor("#2ecc71"));
                } else {
                    ToolTipOverlay::instance()->showText(QCursor::pos(), "移动失败：物理写入未能完成", 2000, QColor("#e81123"));
                }
            }
        });
    });
    return true;
}

bool ContentKeyHandler::handleEvent(QObject* obj, QEvent* event) {
    if (!m_panel) return false;

    switch (event->type()) {
        case QEvent::Wheel:
            return handleWheel(obj, event);
        case QEvent::HoverEnter:
        case QEvent::Enter:
        case QEvent::HoverLeave:
        case QEvent::Leave:
            return handleTooltipHover(obj, event);
        case QEvent::MouseButtonPress:
            return handleMousePress(obj, event);
        case QEvent::KeyPress:
            return handleKeyPress(obj, event);
        default:
            break;
    }
    return false;
}

bool ContentKeyHandler::handleWheel(QObject* obj, QEvent* event) {
    Q_UNUSED(obj);
    QWheelEvent* wEvent = static_cast<QWheelEvent*>(event);
    if (wEvent->modifiers() & Qt::ControlModifier) {
        int deltaY = wEvent->angleDelta().y();
        int newZoom = m_panel->zoomLevel() + (deltaY > 0 ? 8 : -8);
        m_panel->setZoomLevel(newZoom);
        wEvent->accept();
        return true;
    }
    return false;
}

bool ContentKeyHandler::handleTooltipHover(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::HoverEnter || event->type() == QEvent::Enter) {
        QString text = obj->property("tooltipText").toString();
        if (!text.isEmpty()) {
            int timeout = (obj && obj->objectName() == "ViewModeToolBtn") ? 0 : 700;
            ToolTipOverlay::instance()->showText(QCursor::pos(), text, timeout);
        }
    } else if (event->type() == QEvent::HoverLeave || event->type() == QEvent::Leave) {
        ToolTipOverlay::hideTip();
    }
    return false;
}

bool ContentKeyHandler::handleMousePress(QObject* obj, QEvent* event) {
    QMouseEvent* mEvent = static_cast<QMouseEvent*>(event);
    if (mEvent->button() != Qt::LeftButton) return false;

    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(obj);
    if (!view) view = qobject_cast<QAbstractItemView*>(obj->parent());
    if (!view) return false;

    QPoint pos = mEvent->pos();
    if (obj == view && view->viewport()) {
        pos = view->viewport()->mapFrom(view, pos);
    }
    QModelIndex index = view->indexAt(pos);
    if (!index.isValid() || index.data(SectionHeaderRole).toBool()) return false;

    // 1. 网格/自适应视图：归一化 Hitbox 查询
    if (!m_panel->isTreeView(view)) {
        QRect itemRect = view->visualRect(index);
        CardLayout l = CardLayoutEngine::calculate(itemRect, m_panel->zoomLevel());
        int hitVal = l.hitStar(pos);

        if (hitVal != -1) {
            bool isSelected = view->selectionModel() && view->selectionModel()->isSelected(index);
            if (!CardPainterHelper::canModifyRatingOnPress(isSelected, mEvent->modifiers())) {
                return false; // 未选中或有 Ctrl/Shift，不修改评分、不消费事件，放行给视图的默认选中处理
            }

            QAbstractItemModel* model = view->model();
            if (model) {
                // 1. 获取选中的路径集合（多选/单选统一处理）
                QStringList targetPaths;
                if (isSelected && view->selectionModel()) {
                    QModelIndexList selectedIndexes = view->selectionModel()->selectedIndexes();
                    QList<int> selectedRows;
                    for (const QModelIndex& selIdx : selectedIndexes) {
                        if (!selectedRows.contains(selIdx.row())) {
                            selectedRows.append(selIdx.row());
                        }
                    }
                    for (int r : selectedRows) {
                        QModelIndex rowIdx = model->index(r, 0, index.parent());
                        QString p = rowIdx.data(PathRole).toString();
                        if (!p.isEmpty() && !rowIdx.data(SectionHeaderRole).toBool()) {
                            targetPaths.append(p);
                        }
                    }
                }
                if (targetPaths.isEmpty()) {
                    QString p = index.data(PathRole).toString();
                    if (!p.isEmpty()) targetPaths.append(p);
                }

                // 2. 统一分发归一化命令给 CoreEngine
                if (!targetPaths.isEmpty()) {
                    AppCommand cmd;
                    cmd.type = AppCommandType::SetRating;
                    cmd.targetPaths = targetPaths;
                    cmd.params["rating"] = hitVal;
                    CoreEngine::instance().executeCommand(cmd);

                    // 刷新视图上受影响行的 UI 展现
                    for (const QString& p : targetPaths) {
                        m_panel->updateItemMetadata(p);
                    }
                    m_panel->recalculateAndEmitStats();
                }
            }

            QAbstractItemView::EditTriggers cur = view->editTriggers();
            view->setEditTriggers(QAbstractItemView::NoEditTriggers);
            QTimer::singleShot(0, view, [view, cur]() { view->setEditTriggers(cur); });
            event->accept();
            return true;
        }
    }

    // 2. 列表视图（TreeView）第 2 列星级 Hitbox 点击计算
    QTreeView* treeView = qobject_cast<QTreeView*>(view);
    if (treeView) {
        QModelIndex indexCol2 = index.model()->index(index.row(), 2, index.parent());
        QRect col2Rect = treeView->visualRect(indexCol2);

        RatingBarMetrics rm = RatingBarLayout::calculate(col2Rect, RatingBarMode::TreeRow);

        int hitStar = -1;
        if (rm.banRect.contains(pos)) hitStar = 0;
        else {
            for (int i = 0; i < 5; ++i) {
                if (rm.starRect(i).contains(pos)) { hitStar = i + 1; break; }
            }
        }

        if (hitStar != -1) {
            bool isRowSelected = treeView->selectionModel() && treeView->selectionModel()->isRowSelected(index.row(), index.parent());
            if (!CardPainterHelper::canModifyRatingOnPress(isRowSelected, mEvent->modifiers())) {
                return false; // 未选中或有 Ctrl/Shift，不修改评分、不消费事件，放行给视图的默认选中处理
            }

            QAbstractItemModel* model = treeView->model();
            if (model) {
                // 1. 获取选中的路径集合（多选/单选统一处理）
                QStringList targetPaths;
                if (isRowSelected && treeView->selectionModel()) {
                    QModelIndexList selectedIndexes = treeView->selectionModel()->selectedIndexes();
                    QList<int> selectedRows;
                    for (const QModelIndex& selIdx : selectedIndexes) {
                        if (!selectedRows.contains(selIdx.row())) {
                            selectedRows.append(selIdx.row());
                        }
                    }
                    for (int r : selectedRows) {
                        QModelIndex rowIdx = model->index(r, 0, index.parent());
                        QString p = rowIdx.data(PathRole).toString();
                        if (!p.isEmpty() && !rowIdx.data(SectionHeaderRole).toBool()) {
                            targetPaths.append(p);
                        }
                    }
                }
                if (targetPaths.isEmpty()) {
                    QModelIndex targetIdx = model->index(index.row(), 0, index.parent());
                    QString p = targetIdx.data(PathRole).toString();
                    if (!p.isEmpty() && !targetIdx.data(SectionHeaderRole).toBool()) {
                        targetPaths.append(p);
                    }
                }

                // 2. 统一分发归一化命令给 CoreEngine
                if (!targetPaths.isEmpty()) {
                    AppCommand cmd;
                    cmd.type = AppCommandType::SetRating;
                    cmd.targetPaths = targetPaths;
                    cmd.params["rating"] = hitStar;
                    CoreEngine::instance().executeCommand(cmd);

                    // 刷新视图上受影响行的 UI 展现
                    for (const QString& p : targetPaths) {
                        m_panel->updateItemMetadata(p);
                    }
                    m_panel->recalculateAndEmitStats();
                }
            }

            QAbstractItemView::EditTriggers currentTriggers = treeView->editTriggers();
            treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
            QTimer::singleShot(0, treeView, [treeView, currentTriggers]() {
                treeView->setEditTriggers(currentTriggers);
            });
            event->accept();
            return true;
        }
    }
    return false;
}

bool ContentKeyHandler::handleKeyPress(QObject* obj, QEvent* event) {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if (qobject_cast<QLineEdit*>(QApplication::focusWidget())) return false;

    // 面板级全局快捷键（独立于 QAbstractItemView 焦点状态）
    if (keyEvent->modifiers() & Qt::ControlModifier) {
        if (keyEvent->key() == Qt::Key_S) {
            m_panel->toggleFolderSectionCollapse();
            return true;
        }
    }
    if (keyEvent->key() == Qt::Key_Backspace) {
        QDir dir(m_panel->currentPath());
        if (dir.cdUp()) emit m_panel->directorySelected(dir.absolutePath());
        return true;
    }

    QAbstractItemView* view = qobject_cast<QAbstractItemView*>(obj);
    if (!view) view = qobject_cast<QAbstractItemView*>(obj->parent());
    if (!view) return false;

    // 1. Ctrl + 0~5: 评级
    if ((keyEvent->modifiers() & Qt::ControlModifier) && (keyEvent->key() >= Qt::Key_0 && keyEvent->key() <= Qt::Key_5)) {
        int rating = keyEvent->key() - Qt::Key_0;
        QAbstractItemModel* model = view->model();
        if (!model) return true;
        auto indexes = view->selectionModel()->selectedIndexes();
        for (const auto& idx : indexes) {
            if (idx.column() == 0 && !idx.data(SectionHeaderRole).toBool()) model->setData(idx, rating, RatingRole);
        }
        return true;
    }

    // 1.5 Alt + C: 快捷标记/取消标记当前选中的文件夹为目的地
    if (!AppShortcutController::isEditingFocus() && (keyEvent->modifiers() & Qt::AltModifier) && (keyEvent->key() == Qt::Key_C)) {
        QStringList selected = m_panel->getSelectedPaths();
        if (!selected.isEmpty()) {
            for (const QString& p : selected) {
                if (QFileInfo(p).isDir()) {
                    MarkedFolderService::instance().toggleMarked(p);
                }
            }
        }
        return true;
    }

    // 1.6 Alt + V: 将当前选中的项目迁移/发送至所有标记的目的地文件夹
    if (!AppShortcutController::isEditingFocus() && (keyEvent->modifiers() & Qt::AltModifier) && (keyEvent->key() == Qt::Key_V)) {
        QStringList selected = m_panel->getSelectedPaths();
        if (selected.isEmpty()) {
            ToolTipOverlay::instance()->showToast("未选中任何要发送的项目", 2000, QColor("#e81123"));
            return true;
        }

        QStringList markedFolders = MarkedFolderService::instance().getActiveMarkedFolders();
        if (markedFolders.isEmpty()) {
            ToolTipOverlay::instance()->showToast("未设置处于标记状态的目的地文件夹", 2000, QColor("#e81123"));
            return true;
        }

        // 过滤有效目标（排除移动到项目自身、其子目录或当前所在父目录）
        QStringList validTargets;
        for (const QString& target : markedFolders) {
            bool validForAny = false;
            for (const QString& src : selected) {
                QString cleanSrc = QDir::cleanPath(src);
                QString cleanDst = QDir::cleanPath(target);
                QString parentDir = QDir::cleanPath(QFileInfo(src).dir().absolutePath());
                if (cleanSrc.compare(cleanDst, Qt::CaseInsensitive) != 0 &&
                    !cleanDst.startsWith(cleanSrc + "/", Qt::CaseInsensitive) &&
                    cleanDst.compare(parentDir, Qt::CaseInsensitive) != 0) {
                    validForAny = true;
                    break;
                }
            }
            if (validForAny) {
                validTargets << target;
            }
        }

        if (validTargets.isEmpty()) {
            ToolTipOverlay::instance()->showToast("目标文件夹不能是选中的项目本身、其子目录或当前所在父目录", 2000, QColor("#e81123"));
            return true;
        }

        // 同盘/跨盘检测
        bool hasSameVolumeTarget = false;
        for (const QString& src : selected) {
            QStorageInfo srcStorage(src);
            for (const QString& dst : validTargets) {
                QStorageInfo dstStorage(dst);
                if (srcStorage.rootPath().compare(dstStorage.rootPath(), Qt::CaseInsensitive) == 0) {
                    hasSameVolumeTarget = true;
                    break;
                }
            }
            if (hasSameVolumeTarget) break;
        }

        QString modeStr = hasSameVolumeTarget ? "同盘迁移（全部成功后将原文件移至私有回收站）" : "跨盘复制（保留原文件）";
        QString confirmMsg = QString("确定将选中的 %1 个项目发送至以下 %2 个目的地文件夹？\n模式：%3\n\n目的地：\n%4")
                                .arg(selected.size())
                                .arg(validTargets.size())
                                .arg(modeStr)
                                .arg(validTargets.join("\n"));

        FramelessConfirmDialog dlg(m_panel);
        dlg.setTitle("确认发送至目的地");
        dlg.setMessage(confirmMsg);
        dlg.setConfirmText("确定发送");
        dlg.setCancelText("取消");

        if (dlg.exec() != QDialog::Accepted) {
            return true;
        }

        // 定义多目标异步传输共享状态
        struct TransferState {
            QStringList sources;
            QStringList targets;
            bool hasSameVolume = false;
            int totalTargets = 0;
            std::atomic<int> completedTargets{0};
            std::atomic<bool> allSuccess{true};
            int jobId = -1;
            QPointer<ContentPanel> weakPanel;
            QMutex mutex;
        };

        auto state = std::make_shared<TransferState>();
        state->sources = selected;
        state->targets = validTargets;
        state->hasSameVolume = hasSameVolumeTarget;
        state->totalTargets = validTargets.size();
        state->weakPanel = m_panel;
        state->jobId = TaskProgressService::instance().createJob("发送至目的地", validTargets.size());

        m_panel->beginWatchSuppress();

        // 遍历所有目的地调度 DiskIoService 异步拷贝
        for (const QString& targetDir : validTargets) {
            DiskIoContext ioCtx;
            ioCtx.sources = selected;
            ioCtx.destination = targetDir;
            ioCtx.isMove = false; // 统一先复制

            DiskIoService::instance().executeAsync(ioCtx, [state, targetDir](bool success) {
                if (!success) {
                    state->allSuccess = false;
                }
                int done = ++state->completedTargets;
                TaskProgressService::instance().reportProgress(state->jobId, "正在发送至目的地...", done, state->totalTargets);

                // 只有当所有目的地全部执行完毕后，主线程统一收口
                if (done == state->totalTargets) {
                    QMetaObject::invokeMethod(QCoreApplication::instance(), [state]() {
                        bool isAllOk = state->allSuccess.load();
                        bool movedToTrash = false;

                        // 全部目标复制成功且属于同盘迁移时，统一回收源文件至私有回收站
                        if (isAllOk && state->hasSameVolume) {
                            DiskTrashService::moveToDiskTrash(state->sources);
                            movedToTrash = true;
                        }

                        // 压入 UndoManager 支持 Ctrl+Z 撤销
                        class MarkedTransferUndoCommand : public ActionCommand {
                        public:
                            MarkedTransferUndoCommand(QStringList srcs, QStringList dsts, bool trash)
                                : m_srcs(srcs), m_dsts(dsts), m_trash(trash) {}

                            void undo() override {
                                for (const QString& target : m_dsts) {
                                    for (const QString& src : m_srcs) {
                                        QString destFile = QDir(target).filePath(QFileInfo(src).fileName());
                                        if (QFile::exists(destFile)) {
                                            DiskTrashService::moveToDiskTrash({destFile});
                                        }
                                    }
                                }
                                if (m_trash) {
                                    for (const QString& src : m_srcs) {
                                        DiskTrashService::restoreToDirectory(src, QFileInfo(src).dir().absolutePath());
                                    }
                                }
                                CentralEventHub::instance().publishEvent({AppEventType::UndoRedoPerformed, "", {}, {}});
                            }

                            void redo() override {
                                int total = m_dsts.size();
                                auto doneCount = std::make_shared<std::atomic<int>>(0);
                                auto srcs = m_srcs;
                                bool trash = m_trash;

                                for (const QString& target : m_dsts) {
                                    DiskIoContext ioCtx;
                                    ioCtx.sources = m_srcs;
                                    ioCtx.destination = target;
                                    ioCtx.isMove = false;

                                    DiskIoService::instance().executeAsync(ioCtx, [doneCount, total, srcs, trash](bool success) {
                                        if (++(*doneCount) == total) {
                                            QMetaObject::invokeMethod(QCoreApplication::instance(), [srcs, trash, success]() {
                                                if (success && trash) {
                                                    DiskTrashService::moveToDiskTrash(srcs);
                                                }
                                                CentralEventHub::instance().publishEvent({AppEventType::UndoRedoPerformed, "", {}, {}});
                                            });
                                        }
                                    });
                                }
                            }
                            QString description() const override { return "撤销发送至目的地"; }

                        private:
                            QStringList m_srcs;
                            QStringList m_dsts;
                            bool m_trash;
                        };

                        UndoManager::instance().pushCommand(std::make_unique<MarkedTransferUndoCommand>(state->sources, state->targets, movedToTrash));

                        if (state->weakPanel) {
                            state->weakPanel->endWatchSuppress();
                            state->weakPanel->refreshAll();
                        }

                        TaskProgressService::instance().finishJob(state->jobId);

                        if (isAllOk) {
                            ToolTipOverlay::instance()->showToast("已成功发送至目的地", 2000, QColor("#378ADD"));
                        } else {
                            ToolTipOverlay::instance()->showToast("部分目的地发送失败，原文件已保留", 2500, QColor("#e81123"));
                        }
                    });
                }
            });
        }

        return true;
    }

    // 2. Alt + D: 置顶/取消置顶
    if (((keyEvent->modifiers() & Qt::AltModifier) || (keyEvent->modifiers() & (Qt::AltModifier | Qt::WindowShortcut))) && (keyEvent->key() == Qt::Key_D)) {
        auto indexes = view->selectionModel()->selectedIndexes();
        QStringList targetPaths;
        bool anyUnpinned = false;
        for (const QModelIndex& idx : indexes) {
            if (idx.column() == 0 && !idx.data(SectionHeaderRole).toBool()) {
                QString p = idx.data(PathRole).toString();
                if (!p.isEmpty()) {
                    targetPaths << p;
                    if (!idx.data(PinnedRole).toBool()) anyUnpinned = true;
                }
            }
        }
        if (!targetPaths.isEmpty()) {
            ContextMenuFactory::togglePinState(targetPaths, anyUnpinned);
        }
        return true;
    }

    // 3. Alt + 1~9: 色标快速赋予（归一化提交至 CoreEngine）
    if ((keyEvent->modifiers() & Qt::AltModifier) && (keyEvent->key() >= Qt::Key_1 && keyEvent->key() <= Qt::Key_9)) {
        static const QString colors[] = {
            "#E24B4A", "#EF9F27", "#FECF0E", "#639922",
            "#1D9E75", "#378ADD", "#7F77DD", "#5F5E5A", ""
        };
        QString colorValue = colors[keyEvent->key() - Qt::Key_1];

        QAbstractItemModel* model = view->model();
        if (!model || !view->selectionModel()) return true;

        auto indexes = view->selectionModel()->selectedIndexes();
        QStringList selectedPaths;
        for (const auto& idx : indexes) {
            if (idx.column() == 0 && !idx.data(SectionHeaderRole).toBool()) {
                QString p = idx.data(PathRole).toString();
                if (!p.isEmpty()) {
                    selectedPaths.append(p);
                }
            }
        }

        if (!selectedPaths.isEmpty()) {
            AppCommand cmd;
            cmd.type = AppCommandType::SetColor;
            cmd.targetPaths = selectedPaths;
            cmd.params["color"] = colorValue;
            CoreEngine::instance().executeCommand(cmd);

            for (const QString& p : selectedPaths) {
                m_panel->updateItemMetadata(p);
            }
            m_panel->recalculateAndEmitStats();
        }
        return true;
    }

    // 4. Ctrl + Shift + C / V / R
    if ((keyEvent->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)) == (Qt::ControlModifier | Qt::ShiftModifier)) {
        if (keyEvent->key() == Qt::Key_C) {
            // 优先检查选中项目是否有标签，有标签则复制标签；若无标签，则保留原有的复制路径逻辑
            QModelIndex idx = view->currentIndex();
            if (!idx.isValid() && view->selectionModel()) {
                auto selected = view->selectionModel()->selectedIndexes();
                if (!selected.isEmpty()) idx = selected.first();
            }
            if (idx.isValid()) {
                QModelIndex nameIdx = idx.sibling(idx.row(), static_cast<int>(FileListColumn::Name));
                QString path = nameIdx.data(PathRole).toString();
                QString nativePath = QDir::toNativeSeparators(path);
                QStringList tags = MetadataManager::instance().getMeta(nativePath.toStdWString()).tags;
                if (tags.isEmpty()) {
                    tags = nameIdx.data(TagsRole).toStringList();
                }
                tags.removeAll("");
                if (!tags.isEmpty()) {
                    ClipboardService::instance().setCopiedTags(tags);
                    ToolTipOverlay::instance()->showText(QCursor::pos(), QString("已复制 %1 个标签").arg(tags.size()), 1500, QColor("#2ecc71"));
                    return true;
                }
            }

            // Fallback: 复制绝对路径
            QStringList paths;
            auto indexes = view->selectionModel()->selectedIndexes();
            for (const auto& selIdx : indexes) {
                if (selIdx.column() == 0) paths << selIdx.data(PathRole).toString();
            }
            ContextMenuFactory::copyPathsToClipboard(paths, false);
            return true;
        }
        if (keyEvent->key() == Qt::Key_V) {
            ClipboardService::instance().executePasteTags(view);
            return true;
        }
    }

    // 5. F4: 重复上一次操作 (星级 / 标记颜色 / 粘贴标签 / 快捷移入)
    if (keyEvent->key() == Qt::Key_F4) {
        executeRepeatLastOp(m_panel, view);
        return true;
    }

    // 6. 基础文件操作键
    if (keyEvent->key() == Qt::Key_F2) {
        QStringList selectedPaths = m_panel->getSelectedPaths();
        if (selectedPaths.size() > 1) {
            m_panel->performBatchRename();
        } else {
            QModelIndex idx = view->currentIndex();
            if (!idx.isValid() && view->selectionModel()) {
                auto selected = view->selectionModel()->selectedIndexes();
                if (!selected.isEmpty()) idx = selected.first();
            }
            if (idx.isValid()) {
                QModelIndex nameIdx = idx.sibling(idx.row(), static_cast<int>(FileListColumn::Name));
                view->setCurrentIndex(nameIdx);
                view->edit(nameIdx);
            }
        }
        return true;
    }
    if (keyEvent->key() == Qt::Key_Delete) {
        if (keyEvent->modifiers() & Qt::ShiftModifier) {
            PermanentDeleteService::instance().execute(m_panel->getSelectedPaths(), m_panel);
        } else {
            TrashService::instance().moveToTrash(m_panel->getSelectedPaths(), m_panel);
        }
        return true;
    }

    // 6. Ctrl + C / X / V
    if (keyEvent->modifiers() & Qt::ControlModifier) {
        if (keyEvent->key() == Qt::Key_C && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            m_panel->performCopy(false);
            return true;
        }
        if (keyEvent->key() == Qt::Key_X) {
            m_panel->performCopy(true);
            return true;
        }
        if (keyEvent->key() == Qt::Key_V) {
            m_panel->performPaste();
            return true;
        }
    }

    // 7. 空格键与回车键: 统一激活当前项（QuickLook 预览或打开文件夹）
    if (keyEvent->key() == Qt::Key_Space || keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
        QModelIndex idx = view->currentIndex();
        if (!idx.isValid() && view->selectionModel()) {
            auto selected = view->selectionModel()->selectedIndexes();
            if (!selected.isEmpty()) idx = selected.first();
        }
        if (idx.isValid()) {
            QModelIndex nameIdx = idx.sibling(idx.row(), static_cast<int>(FileListColumn::Name));
            m_panel->activateIndex(nameIdx);
        }
        return true;
    }
    if ((keyEvent->modifiers() & Qt::ControlModifier) && keyEvent->key() == Qt::Key_Backslash) {
        ContentPanel::ViewMode nextMode = ContentPanel::ListView;
        if (m_panel->currentViewMode() == ContentPanel::ListView) nextMode = ContentPanel::GridView;
        else if (m_panel->currentViewMode() == ContentPanel::GridView) nextMode = ContentPanel::JustifiedViewMode;
        else if (m_panel->currentViewMode() == ContentPanel::JustifiedViewMode) nextMode = ContentPanel::ListView;
        m_panel->setViewMode(nextMode);
        return true;
    }

    return false;
}

} // namespace QuarkMeta