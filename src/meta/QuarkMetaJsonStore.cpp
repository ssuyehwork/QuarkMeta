#include "QuarkMetaJsonStore.h"

namespace QuarkMeta {

QuarkMetaJsonStore& QuarkMetaJsonStore::instance() {
    static QuarkMetaJsonStore inst;
    return inst;
}

bool QuarkMetaJsonStore::updateItemMeta(const std::wstring& path, std::function<void(ItemMeta&)> modifier) {
    return QuarkMetaJson::updateItemMeta(path, modifier);
}

bool QuarkMetaJsonStore::renameItem(const QString& folderPath, const QString& oldName, const QString& newName) {
    return QuarkMetaJson::renameItem(folderPath, oldName, newName);
}

bool QuarkMetaJsonStore::migrateFolderCache(const QString& oldFolderPath, const QString& newFolderPath) {
    return QuarkMetaJson::migrateFolderCache(oldFolderPath, newFolderPath);
}

} // namespace QuarkMeta
