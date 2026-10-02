#include "FileCreationService.h"
#include "../ui/ContentPanel.h"
#include "../ui/models/DiskItemModel.h"
#include "../core/ItemRecord.h"
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QDebug>

namespace QuarkMeta {

FileCreationService& FileCreationService::instance() {
    static FileCreationService inst;
    return inst;
}

FileCreationService::FileCreationService(QObject* parent) : QObject(parent) {}

bool FileCreationService::createNewItem(ContentPanel* panel, const QString& type) {
    if (!panel) return false;
    QString currentPath = panel->activePath();
    if (currentPath.isEmpty() || currentPath == "computer://") return false;

    QString baseName = (type == "folder") ? "新建文件夹" : "未命名";
    QString ext = (type == "md") ? ".md" : ((type == "txt") ? ".txt" : "");
    QString finalName = baseName + ext;
    QString fullPath = QDir(currentPath).filePath(finalName);
    int counter = 1;

    while (QFileInfo::exists(fullPath)) {
        finalName = baseName + QString(" (%1)").arg(counter++) + ext;
        fullPath = QDir(currentPath).filePath(finalName);
    }

    bool success = false;
    if (type == "folder") {
        success = QDir(currentPath).mkdir(finalName);
    } else {
        QFile f(fullPath);
        if (f.open(QIODevice::WriteOnly)) {
            f.close();
            success = true;
        }
    }

    if (!success) return false;

    qDebug() << "[CREATE_ITEM_DIAG] 新建类型:" << type << "| 目标路径:" << fullPath << "| 物理创建成功";

    // 1. 同步将新项目追加至 Model，彻底消除全盘异步扫描的时序脱节与状态遗失
    ItemRecord newRec = ItemRecord::create(fullPath);
    if (panel->model()) {
        panel->model()->appendRecord(newRec);
        qDebug() << "[CREATE_ITEM_DIAG] 已向 Model 追加新记录，当前记录总数:" << panel->model()->allRecords().size();
    }
    panel->applyFilters();

    // 2. 强锁定焦点并即时触发 Delegate 代理重命名编辑框 (100% 稳固)
    qDebug() << "[CREATE_ITEM_DIAG] 准备调用 selectAndEditPath...";
    panel->selectAndEditPath(fullPath);
    return true;
}

} // namespace QuarkMeta
