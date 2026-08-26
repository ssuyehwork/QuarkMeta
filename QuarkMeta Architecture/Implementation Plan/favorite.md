# Implementation Plan - Favorite Panel Duplicate Prevention Fix

## 1. Overview
This implementation plan addresses the bug where duplicate items (files/folders) can be added to the Favorite Panel (`FavoritePanel`). It enforces absolute physical path uniqueness, path normalization (`QDir::cleanPath` and `QDir::toNativeSeparators`), and prevents Qt's default model drop insertion from bypassing path uniqueness checks.

## 2. Modified Files List
- `src/ui/FavoritePanel.cpp`
- `src/ui/FavoritePanel.h`

## 3. Detailed Line-by-Line Changes

```diff
<<<<<<< SEARCH
void FavoritePanel::addFavoriteItem(const QString& path) {
    for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
        if (m_favoriteModel->item(i)->data(Qt::UserRole + 1).toString() == path) {
            return;
        }
    }

    QFileInfo fi(path);
    if (!fi.exists()) return;

    QIcon icon = ShellIconManager::getFileIcon(path, 18);
    QStandardItem* item = new QStandardItem(icon, fi.fileName().isEmpty() ? path : fi.fileName());
    item->setData(path, Qt::UserRole + 1);

    m_favoriteModel->appendRow(item);
}
=======
void FavoritePanel::addFavoriteItem(const QString& path) {
    QString cleanPath = QDir::toNativeSeparators(QDir::cleanPath(path));
    if (cleanPath.isEmpty()) return;

    for (int i = 0; i < m_favoriteModel->rowCount(); ++i) {
        QString existingPath = QDir::toNativeSeparators(QDir::cleanPath(m_favoriteModel->item(i)->data(Qt::UserRole + 1).toString()));
        if (QString::compare(existingPath, cleanPath, Qt::CaseInsensitive) == 0) {
            return;
        }
    }

    QFileInfo fi(cleanPath);
    if (!fi.exists()) return;

    QIcon icon = ShellIconManager::getFileIcon(cleanPath, 18);
    QStandardItem* item = new QStandardItem(icon, fi.fileName().isEmpty() ? cleanPath : fi.fileName());
    item->setData(cleanPath, Qt::UserRole + 1);

    m_favoriteModel->appendRow(item);
}
>>>>>>> REPLACE
```

```diff
<<<<<<< SEARCH
void FavoritePanel::loadFavorites() {
    if (!m_favoriteModel) return;
    m_favoriteModel->clear();

    QVariant val = AppConfig::instance().getValue("FavoritePanel/Favorites");
    if (!val.isValid()) {
        val = AppConfig::instance().getValue("NavPanel/Favorites"); // 向下兼容原配置
    }
    if (!val.isValid()) return;

    QJsonDocument doc = QJsonDocument::fromJson(val.toByteArray());
    if (!doc.isArray()) return;

    QJsonArray arr = doc.array();
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject obj = arr.at(i).toObject();
        QString path = obj.value("path").toString();
        if (!path.isEmpty()) {
            addFavoriteItem(path);
        }
    }
}
=======
void FavoritePanel::loadFavorites() {
    if (!m_favoriteModel) return;
    m_favoriteModel->clear();

    QVariant val = AppConfig::instance().getValue("FavoritePanel/Favorites");
    if (!val.isValid()) {
        val = AppConfig::instance().getValue("NavPanel/Favorites"); // 向下兼容原配置
    }
    if (!val.isValid()) return;

    QJsonDocument doc = QJsonDocument::fromJson(val.toByteArray());
    if (!doc.isArray()) return;

    QJsonArray arr = doc.array();
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject obj = arr.at(i).toObject();
        QString path = obj.value("path").toString();
        if (!path.isEmpty()) {
            addFavoriteItem(path);
        }
    }
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Rebuild application using CMake & Qt build pipeline.
2. Drag and drop the same folder into the Favorite Panel multiple times. Verify that only one entry exists.
3. Attempt adding paths with different slashes (e.g., `C:/Folder` vs `C:\Folder`). Verify no duplicate entries are added.
