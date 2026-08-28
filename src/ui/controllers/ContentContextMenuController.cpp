#include "ContentContextMenuController.h"
#include "../ContentPanel.h"
#include "../UiHelper.h"
#include "../FavoritePanel.h"
#include "../ColorPicker.h"
#include "../BatchCreateDialog.h"
#include "../BatchRenameDialog.h"
#include "../../core/TrashService.h"
#include "../../core/PermanentDeleteService.h"
#include "../../core/ClipboardService.h"
#include "../../crypto/EncryptionManager.h"
#include "../../core/OperationSnapshotEngine.h"
#include "../../core/AppConfig.h"
#include "../../core/ModelContract.h"
#include "../../util/ShellHelper.h"
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QWidgetAction>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QApplication>

namespace QuarkMeta {

ContentContextMenuController::ContentContextMenuController(ContentPanel* panel, QObject* parent)
    : QObject(parent), m_panel(panel) {}

void ContentContextMenuController::showContextMenu(QAbstractItemView* view,
                                                   const QPoint& pos,
                                                   const QString& currentPath,
                                                   const QString& categoryType) {
    if (!view || !m_panel) return;

    QModelIndex currentIndex = view->indexAt(pos);
    bool onItem = currentIndex.isValid();
    QString path = onItem ? currentIndex.data(PathRole).toString() : "";
    QFileInfo itemInfo(path);

    bool isComputerRoot = (currentPath.isEmpty() || currentPath == "computer://");
    bool isTrashView = (categoryType == "trash" || currentPath == "trash://");
    bool isDriveRoot = onItem && (itemInfo.isRoot() || path.endsWith(":\\") || path.endsWith(":/") || (path.length() == 2 && path.endsWith(':')));
    bool isFolder = onItem && (isDriveRoot || currentIndex.data(TypeRole).toString() == "folder");

    QMenu menu;
    UiHelper::applyMenuStyle(&menu);

    // =========================================================================
    // 场景 1：回收站视图
    // =========================================================================
    if (isTrashView) {
        if (onItem) {
            menu.addAction(UiHelper::getIcon("sync", QColor("#2ecc71"), 18), "还原")->setData(ContentPanel::ActionRestore);
            menu.addAction(UiHelper::getIcon("cut", QColor("#EEEEEE"), 18), "剪切")->setData(ContentPanel::ActionCut);
            menu.addAction(UiHelper::getIcon("trash", QColor("#e81123"), 18), "永久删除")->setData(ContentPanel::ActionSecureDelete);
            menu.addSeparator();
        }
        menu.addAction(UiHelper::getIcon("sync", QColor("#2ecc71"), 18), "还原全部")->setData(ContentPanel::ActionRestoreAll);
        menu.addAction(UiHelper::getIcon("trash", QColor("#e81123"), 18), "清空回收站")->setData(ContentPanel::ActionEmptyTrash);

        QAction* selected = menu.exec(view->viewport()->mapToGlobal(pos));
        if (!selected || !selected->data().isValid()) return;

        auto action = static_cast<ContentPanel::ContextAction>(selected->data().toInt());
        if (action == ContentPanel::ActionRestore) {
            TrashService::instance().restoreItems(m_panel->getSelectedTrashIds(), m_panel);
        } else if (action == ContentPanel::ActionRestoreAll) {
            TrashService::instance().restoreAll(m_panel);
        } else if (action == ContentPanel::ActionEmptyTrash) {
            TrashService::instance().emptyTrash(m_panel);
        } else if (action == ContentPanel::ActionCut) {
            ClipboardService::instance().cutItems(m_panel->getSelectedPaths());
        } else if (action == ContentPanel::ActionSecureDelete) {
            PermanentDeleteService::instance().execute(m_panel->getSelectedPaths(), m_panel);
        }
        return;
    }

    // =========================================================================
    // 场景 2：选中具体文件或目录
    // =========================================================================
    if (onItem) {
        if (isDriveRoot) {
            menu.addAction("打开")->setData(ContentPanel::ActionOpen);
            menu.addAction("在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);

            // 颜色条
            QString currentColorStr = currentIndex.data(ColorRole).toString();
            QWidgetAction* pickerAction = new QWidgetAction(&menu);
            ColorStripPicker* pickerWidget = new ColorStripPicker(currentColorStr, &menu);
            pickerAction->setDefaultWidget(pickerWidget);
            menu.addAction(pickerAction);

            connect(pickerWidget, &ColorStripPicker::colorSelected, m_panel, [this, view, &menu](const QString& hexColor) {
                for (const auto& idx : view->selectionModel()->selectedIndexes()) {
                    if (idx.column() == 0) view->model()->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });

            bool isPinned = currentIndex.data(PinnedRole).toBool();
            menu.addAction(isPinned ? "取消置顶" : "置顶")->setData(isPinned ? ContentPanel::ActionUnpin : ContentPanel::ActionPin);
            menu.addAction("添加至收藏夹 / 切换收藏")->setData(ContentPanel::ActionAddToFavorites);
            menu.addSeparator();

            QAction* actPaste = menu.addAction("粘贴");
            actPaste->setData(ContentPanel::ActionPaste);
            actPaste->setEnabled(ClipboardService::instance().canPaste(path));

            menu.addAction("复制名称")->setData(ContentPanel::ActionCopyName);
            menu.addAction("复制路径")->setData(ContentPanel::ActionCopyPath);
            menu.addSeparator();
            menu.addAction("刷新")->setData(ContentPanel::ActionRefresh);
        } else {
            menu.addAction(isFolder ? "打开文件夹" : "打开")->setData(ContentPanel::ActionOpen);
            if (!isFolder) menu.addAction("用系统默认程序打开")->setData(ContentPanel::ActionOpenDefault);
            menu.addAction("在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);

            // 颜色条
            QString currentColorStr = currentIndex.data(ColorRole).toString();
            QWidgetAction* pickerAction = new QWidgetAction(&menu);
            ColorStripPicker* pickerWidget = new ColorStripPicker(currentColorStr, &menu);
            pickerAction->setDefaultWidget(pickerWidget);
            menu.addAction(pickerAction);

            connect(pickerWidget, &ColorStripPicker::colorSelected, m_panel, [this, view, &menu](const QString& hexColor) {
                for (const auto& idx : view->selectionModel()->selectedIndexes()) {
                    if (idx.column() == 0) view->model()->setData(idx, hexColor, ColorRole);
                }
                menu.close();
            });

            bool isPinned = currentIndex.data(PinnedRole).toBool();
            menu.addAction(isPinned ? "取消置顶" : "置顶")->setData(isPinned ? ContentPanel::ActionUnpin : ContentPanel::ActionPin);
            menu.addAction("添加至收藏夹 / 切换收藏")->setData(ContentPanel::ActionAddToFavorites);
            menu.addSeparator();

            menu.addAction("复制")->setData(ContentPanel::ActionCopy);
            menu.addAction("剪切")->setData(ContentPanel::ActionCut);

            QAction* actPaste = menu.addAction("粘贴");
            actPaste->setData(ContentPanel::ActionPaste);
            actPaste->setEnabled(ClipboardService::instance().canPaste(isFolder ? path : currentPath));

            menu.addAction("复制名称")->setData(ContentPanel::ActionCopyName);
            menu.addAction("复制路径")->setData(ContentPanel::ActionCopyPath);

            int selCount = m_panel->getSelectedPaths().size();
            if (selCount <= 1) menu.addAction("重命名")->setData(ContentPanel::ActionRename);
            if (isFolder || selCount > 1) menu.addAction("批量重命名 (Ctrl+Shift+R)")->setData(ContentPanel::ActionBatchRename);

            menu.addSeparator();
            menu.addAction("刷新")->setData(ContentPanel::ActionRefresh);

            if (!isFolder) {
                menu.addAction(UiHelper::getIcon("sync", QColor("#3498db"), 18), "重新提取缩略图")->setData(ContentPanel::ActionReextractThumbnail);
                QMenu* cryptoMenu = menu.addMenu("外壳保护");
                UiHelper::applyMenuStyle(cryptoMenu);
                cryptoMenu->addAction("执行外壳保护")->setData(ContentPanel::ActionEncrypt);
                cryptoMenu->addAction("解除保护")->setData(ContentPanel::ActionDecrypt);
                cryptoMenu->addAction("修改保护密码")->setData(ContentPanel::ActionChangePwd);
            }
        }
    }
    // =========================================================================
    // 场景 3：空白处
    // =========================================================================
    else {
        if (isComputerRoot) {
            menu.addAction("在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);
            menu.addAction("刷新")->setData(ContentPanel::ActionRefresh);
        } else {
            QMenu* newMenu = menu.addMenu("新建...");
            UiHelper::applyMenuStyle(newMenu);
            newMenu->addAction(UiHelper::getIcon("folder_filled", QColor("#EEEEEE")), "创建文件夹")->setData(ContentPanel::ActionNewFolder);
            newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建 Markdown")->setData(ContentPanel::ActionNewMd);
            newMenu->addAction(UiHelper::getIcon("text", QColor("#EEEEEE")), "创建纯文本文件 (txt)")->setData(ContentPanel::ActionNewTxt);

            menu.addSeparator();
            menu.addAction(UiHelper::getIcon("add", QColor("#EEEEEE")), "批量创建项目...")->setData(ContentPanel::ActionBatchCreate);
            menu.addSeparator();

            QAction* actPaste = menu.addAction("粘贴");
            actPaste->setData(ContentPanel::ActionPaste);
            actPaste->setEnabled(ClipboardService::instance().canPaste(currentPath));

            menu.addSeparator();
            menu.addAction("在“资源管理器”中显示")->setData(ContentPanel::ActionShowInExplorer);
            menu.addAction("刷新")->setData(ContentPanel::ActionRefresh);
        }
    }

    // 排序二级子菜单
    menu.addSeparator();
    QMenu* sortMenu = menu.addMenu("排序");
    UiHelper::applyMenuStyle(sortMenu);

    QActionGroup* typeGroup = new QActionGroup(m_panel);
    auto addTypeAct = [&](const QString& label, ContentPanel::SortType type) {
        QAction* act = sortMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_panel->currentSortType() == type);
        typeGroup->addAction(act);
        connect(act, &QAction::triggered, m_panel, [this, type]() {
            m_panel->setSortType(type);
        });
    };

    addTypeAct("名称", ContentPanel::SortByName);
    addTypeAct("创建日期", ContentPanel::SortByCreateDate);
    addTypeAct("修改日期", ContentPanel::SortByModifyDate);
    addTypeAct("扩展名", ContentPanel::SortByExtension);
    addTypeAct("大小", ContentPanel::SortBySize);
    addTypeAct("尺寸", ContentPanel::SortByDimension);
    addTypeAct("评分", ContentPanel::SortByRating);

    sortMenu->addSeparator();
    QActionGroup* orderGroup = new QActionGroup(m_panel);
    auto addOrderAct = [&](const QString& label, Qt::SortOrder order) {
        QAction* act = sortMenu->addAction(label);
        act->setCheckable(true);
        act->setChecked(m_panel->currentSortOrder() == order);
        orderGroup->addAction(act);
        connect(act, &QAction::triggered, m_panel, [this, order]() {
            m_panel->setSortOrder(order);
        });
    };
    addOrderAct("升序", Qt::AscendingOrder);
    addOrderAct("降序", Qt::DescendingOrder);

    // 删除子菜单
    if (onItem && !isDriveRoot) {
        menu.addSeparator();
        QMenu* delMenu = menu.addMenu("删除");
        UiHelper::applyMenuStyle(delMenu);
        delMenu->addAction("移入回收站")->setData(ContentPanel::ActionDelete);
        delMenu->addAction("永久删除")->setData(ContentPanel::ActionSecureDelete);
    }

    QAction* selected = menu.exec(view->viewport()->mapToGlobal(pos));
    if (!selected || !selected->data().isValid()) return;

    auto action = static_cast<ContentPanel::ContextAction>(selected->data().toInt());

    // 🚀【统一领域调度】：彻底消灭私有硬编码
    switch (action) {
        case ContentPanel::ActionOpen: m_panel->onDoubleClicked(currentIndex); break;
        case ContentPanel::ActionOpenDefault: {
            for (const QString& p : m_panel->getSelectedPaths()) {
                QDesktopServices::openUrl(QUrl::fromLocalFile(p));
            }
            break;
        }
        case ContentPanel::ActionShowInExplorer: ShellHelper::openInExplorer(onItem ? path : currentPath); break;
        case ContentPanel::ActionNewFolder: m_panel->createNewItem("folder"); break;
        case ContentPanel::ActionNewMd: m_panel->createNewItem("md"); break;
        case ContentPanel::ActionNewTxt: m_panel->createNewItem("txt"); break;
        case ContentPanel::ActionPin:
        case ContentPanel::ActionUnpin: {
            bool pin = (action == ContentPanel::ActionPin);
            for (const auto& idx : view->selectionModel()->selectedIndexes()) {
                if (idx.column() == 0) view->model()->setData(idx, pin, PinnedRole);
            }
            break;
        }
        case ContentPanel::ActionEncrypt:
        case ContentPanel::ActionDecrypt:
        case ContentPanel::ActionChangePwd: break;
        case ContentPanel::ActionBatchRename: m_panel->performBatchRename(); break;
        case ContentPanel::ActionRename: view->edit(currentIndex); break;
        case ContentPanel::ActionCopy: ClipboardService::instance().copyItems(m_panel->getSelectedPaths()); break;
        case ContentPanel::ActionCut: ClipboardService::instance().cutItems(m_panel->getSelectedPaths()); break;
        case ContentPanel::ActionPaste: ClipboardService::instance().executePaste(isFolder ? path : currentPath, m_panel); break;
        case ContentPanel::ActionBatchCreate: {
            BatchCreateDialog dlg(currentPath, m_panel);
            if (dlg.exec() == QDialog::Accepted) m_panel->refreshAll();
            break;
        }
        case ContentPanel::ActionDelete: TrashService::instance().moveToTrash(m_panel->getSelectedPaths(), m_panel); break;
        case ContentPanel::ActionSecureDelete: PermanentDeleteService::instance().execute(m_panel->getSelectedPaths(), m_panel); break;
        case ContentPanel::ActionAddToFavorites: emit m_panel->requestAddFavorite(m_panel->getSelectedPaths()); break;
        case ContentPanel::ActionCopyName: {
            QStringList names;
            for (const QString& p : m_panel->getSelectedPaths()) names << QFileInfo(p).fileName();
            QApplication::clipboard()->setText(names.join("\r\n"));
            break;
        }
        case ContentPanel::ActionCopyPath: {
            QApplication::clipboard()->setText(m_panel->getSelectedPaths().join("\n"));
            break;
        }
        case ContentPanel::ActionRefresh: m_panel->refreshAll(); break;
        default: break;
    }
}

} // namespace QuarkMeta
