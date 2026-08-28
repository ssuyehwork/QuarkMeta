#pragma once
#include <QString>
#include <QStringList>

namespace QuarkMeta {

/**
 * @brief 系统服务层工具类 (ShellHelper)
 * 封装 Windows 原生 Shell 调用与通用格式化逻辑。
 */
class ShellHelper {
public:
    /**
     * @brief 在 Windows 文件资源管理器中高亮定位指定物理路径
     */
    static void openInExplorer(const QString& path);

    /**
     * @brief 呼出 Windows 原生文件属性对话框
     */
    static void showProperties(const QString& path);

    /**
     * @brief 两阶段 UUID 安全重命名 (解决 NTFS 大小写不敏感缺陷并自动漫游元数据)
     */
    static bool renameItem(const QString& oldPath, const QString& newPath);

    /**
     * @brief 格式化字节大小为易读文本 (B / KB / MB / GB)
     */
    static QString formatSize(qint64 bytes);

    /**
     * @brief 物理赋予文件/文件夹 Windows 隐藏属性
     */
    static void ensureHidden(const std::wstring& path);
};

} // namespace QuarkMeta
