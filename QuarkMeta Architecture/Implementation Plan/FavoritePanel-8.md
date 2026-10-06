# FavoritePanel Implementation Plan - Invalid Path Handling on Click

This implementation plan outlines the non-destructive invalid path handling for favorite items when clicked:
- When a user clicks a favorite item whose target path no longer exists on disk (`!QFileInfo::exists(path)`), a confirmation dialog (`FramelessConfirmDialog`) is shown.
- If the user confirms removal, the item is deleted from the SQLite database (`FavoriteDao::deleteFavorite`) and removed from the UI model.
- If the user chooses to keep the item (e.g., unplugged USB drive or temporarily unmounted network share), no changes are made.

---

## 1. Overview
Currently, clicking a favorite item in `FavoritePanel::onFavoriteClicked` emits `directorySelected` or `requestLocateFile` directly without verifying whether `path` exists on disk. If the underlying file or folder has been moved, renamed, or deleted, clicking the favorite item either does nothing or causes navigation errors.

To improve reliability and protect users with removable drives:
1. `FavoritePanel::onFavoriteClicked` will perform a runtime check via `QFileInfo::exists(path)`.
2. If the path does not exist, `FramelessConfirmDialog` will pop up asking:
   > "无法找到路径 %1，该文件或文件夹可能已被移动、重命名或删除。\n\n是否将其从收藏夹中移除？"
3. If confirmed (`QDialog::Accepted`), `FavoriteDao::deleteFavorite(recId)` is executed, `removeFavoriteItem(path)` updates the tree model, and `FavoritePanel::saveFavorites()` persists the state.
4. If cancelled (`QDialog::Rejected`), the favorite entry remains intact.

---

## 2. Modified Files List
- `src/ui/FavoritePanel.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/FavoritePanel.cpp`

```
<<<<<<< SEARCH
#include "FavoritePanel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "../meta/FavoriteDao.h"
#include "../meta/MetadataManager.h"
#include "../core/ThumbnailPipelineService.h"
#include "../core/DiskMediaExtractor.h"
#include <QMenu>
#include <QDir>
#include <QFileInfo>
#include <QPointer>
#include <QtConcurrent>
#include <QCoreApplication>
=======
#include "FavoritePanel.h"
#include "UiHelper.h"
#include "ShellIconManager.h"
#include "dialogs/FramelessConfirmDialog.h"
#include "../meta/FavoriteDao.h"
#include "../meta/MetadataManager.h"
#include "../core/ThumbnailPipelineService.h"
#include "../core/DiskMediaExtractor.h"
#include <QMenu>
#include <QDir>
#include <QFileInfo>
#include <QPointer>
#include <QtConcurrent>
#include <QCoreApplication>
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void FavoritePanel::onFavoriteClicked(const QModelIndex& index) {
    bool isVirtual = index.data(Qt::UserRole + 7).toBool();
    if (isVirtual) {
        if (m_favoriteView) {
            if (m_favoriteView->isExpanded(index)) {
                m_favoriteView->collapse(index);
            } else {
                m_favoriteView->expand(index);
            }
        }
        return;
    }

    QString path = index.data(Qt::UserRole + 1).toString();
    if (path.isEmpty()) return;

    QFileInfo fi(path);
    if (fi.isDir()) {
        emit directorySelected(path);
    } else {
        emit requestLocateFile(path);
    }
}
=======
void FavoritePanel::onFavoriteClicked(const QModelIndex& index) {
    bool isVirtual = index.data(Qt::UserRole + 7).toBool();
    if (isVirtual) {
        if (m_favoriteView) {
            if (m_favoriteView->isExpanded(index)) {
                m_favoriteView->collapse(index);
            } else {
                m_favoriteView->expand(index);
            }
        }
        return;
    }

    QString path = index.data(Qt::UserRole + 1).toString();
    if (path.isEmpty()) return;

    QFileInfo fi(path);
    if (!fi.exists()) {
        int recId = index.data(Qt::UserRole + 6).toInt();
        QString msg = QString("无法找到路径：%1\n该文件或文件夹可能已被移动、重命名或删除。\n\n是否将其从收藏夹中移除？").arg(path);
        FramelessConfirmDialog dlg("提示", msg, FramelessConfirmDialog::OkCancel, "alert_warning", QColor("#e74c3c"), this);
        if (dlg.exec() == QDialog::Accepted) {
            if (recId > 0) {
                FavoriteDao::deleteFavorite(recId);
            }
            removeFavoriteItem(path);
            saveFavorites();
        }
        return;
    }

    if (fi.isDir()) {
        emit directorySelected(path);
    } else {
        emit requestLocateFile(path);
    }
}
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
Since Qt6 dependencies are not available in the headless sandbox, compilation is skipped.
Functional verification checklist for native build:
1. Add a temporary file or folder to FavoritePanel.
2. Externally delete or rename the target file/folder on disk.
3. Single-click the item in FavoritePanel.
4. Verify `FramelessConfirmDialog` appears with the warning message.
5. Click **[取消]** (Cancel). Verify the item remains in FavoritePanel.
6. Single-click the item again and click **[确定]** (Ok). Verify the item is removed from FavoritePanel and deleted from SQLite database.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Dialog SSOT**: Uses existing `FramelessConfirmDialog` from `src/ui/dialogs/FramelessConfirmDialog.h` for consistent frameless dialog styling.
- **DAO SSOT**: Reuses `FavoriteDao::deleteFavorite(recId)` for database removal.
- **Tree Model SSOT**: Reuses existing `removeFavoriteItem(path)` and `saveFavorites()` helpers.

---

## 6. Header API Signature Verification
- `FavoriteDao::deleteFavorite(int id)` -> `src/meta/FavoriteDao.h`
- `FavoritePanel::removeFavoriteItem(const QString& path)` -> `src/ui/FavoritePanel.h`
- `FavoritePanel::saveFavorites()` -> `src/ui/FavoritePanel.h`
- `FramelessConfirmDialog::FramelessConfirmDialog(const QString& title, const QString& message, ButtonType type, const QString& iconName, const QColor& iconColor, QWidget* parent)` -> `src/ui/dialogs/FramelessConfirmDialog.h`

---

## 7. Header Inclusion Chain & Type Completeness Check
- `#include "dialogs/FramelessConfirmDialog.h"` added to `src/ui/FavoritePanel.cpp`.
- `#include <QFileInfo>` is already present in `src/ui/FavoritePanel.cpp`.
