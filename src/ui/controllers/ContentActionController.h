#pragma once

#include <QObject>
#include <QStringList>
#include <QWidget>

namespace QuarkMeta {

class ContentActionController : public QObject {
    Q_OBJECT

public:
    explicit ContentActionController(QObject* parent = nullptr) : QObject(parent) {}
    ~ContentActionController() override = default;

    /**
     * @brief 批量创建文件或目录
     */
    bool createNewItem(const QString& currentDir, const QString& type, QString& outCreatedPath);
};

} // namespace QuarkMeta
