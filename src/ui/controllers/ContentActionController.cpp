#include "ContentActionController.h"
#include <QDir>
#include <QFileInfo>
#include <QFile>

namespace QuarkMeta {

bool ContentActionController::createNewItem(const QString& currentDir, const QString& type, QString& outCreatedPath) {
    if (currentDir.isEmpty() || currentDir == "computer://" || currentDir.contains("://")) return false;

    QString baseName = (type == "folder") ? "新建文件夹" : "未命名";
    QString ext = (type == "md") ? ".md" : ((type == "txt") ? ".txt" : "");
    QString finalName = baseName + ext;
    QString fullPath = currentDir + "/" + finalName;

    int counter = 1;
    while (QFileInfo::exists(fullPath)) {
        finalName = baseName + QString(" (%1)").arg(counter++) + ext;
        fullPath = currentDir + "/" + finalName;
    }

    bool success = false;
    if (type == "folder") {
        success = QDir(currentDir).mkdir(finalName);
    } else {
        QFile file(fullPath);
        if (file.open(QIODevice::WriteOnly)) {
            file.close();
            success = true;
        }
    }

    if (success) {
        outCreatedPath = finalName;
    }
    return success;
}

} // namespace QuarkMeta
