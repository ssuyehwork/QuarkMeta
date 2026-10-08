# MetaPanel Implementation Plan

## 1. Overview
This implementation plan addresses two specific user requirements for `MetaPanel` and its wiring in `PanelMediator`:
1. **Dimension Display Restoration**: Restore the display of image/media dimensions (e.g. `1920 x 1080 像素`) in `MetaPanel`'s "基础属性" (Basic Attributes) section by passing real cached `meta.width` and `meta.height` values from `MetadataManager` in `PanelMediator.cpp`, instead of passing hardcoded `0, 0`.
2. **Top Preview Icon Size Restriction**: When `setImagePreview` receives a default file/folder icon (non-thumbnail, indicated when `HasThumbnailRole` is false or when the icon is rendered as a default icon), restrict its maximum dimensions to **35x45 pixels** (max width 45, max height 35) to match the visual proportions of file icons in content cards.

---

## 2. Modified Files List
1. `src/ui/MetaPanel.h`
2. `src/ui/MetaPanel.cpp`
3. `src/ui/PanelMediator.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/ui/MetaPanel.h`
Add an overload or bool flag `isDefaultIcon` to `setImagePreview` so that `MetaPanel` can distinguish between a real image thumbnail and a default file icon.

```
<<<<<<< SEARCH
    void setImagePreview(const QPixmap& pixmap);
=======
    void setImagePreview(const QPixmap& pixmap, bool isDefaultIcon = false);
>>>>>>> REPLACE
```

---

### Change 2: `src/ui/MetaPanel.cpp`
Update `MetaPanel::setImagePreview` implementation to handle `isDefaultIcon` by capping its scale to `45x35` pixels.

```
<<<<<<< SEARCH
void MetaPanel::setImagePreview(const QPixmap& pixmap) {
    if (!m_lblImagePreview) return;
    if (pixmap.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
        if (m_topPreviewBox) m_topPreviewBox->hide();
    } else {
        int maxW = m_container ? (m_container->width() - 16) : 214;
        maxW = qBound(120, maxW, 230);
        int maxH = 220;

        QPixmap scaled = (pixmap.width() > maxW || pixmap.height() > maxH)
            ? pixmap.scaled(QSize(maxW, maxH), Qt::KeepAspectRatio, Qt::SmoothTransformation)
            : pixmap;
=======
void MetaPanel::setImagePreview(const QPixmap& pixmap, bool isDefaultIcon) {
    if (!m_lblImagePreview) return;
    if (pixmap.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
        if (m_topPreviewBox) m_topPreviewBox->hide();
    } else {
        int maxW = m_container ? (m_container->width() - 16) : 214;
        maxW = qBound(120, maxW, 230);
        int maxH = 220;

        if (isDefaultIcon) {
            maxW = 45;
            maxH = 35;
        }

        QPixmap scaled = (pixmap.width() > maxW || pixmap.height() > maxH)
            ? pixmap.scaled(QSize(maxW, maxH), Qt::KeepAspectRatio, Qt::SmoothTransformation)
            : pixmap;
>>>>>>> REPLACE
```

---

### Change 3: `src/ui/PanelMediator.cpp`
1. In `updateMetaPanelFromPanel`, fetch `meta.width` and `meta.height` from `MetadataManager::instance().getMeta(path.toStdWString())` and pass them to `metaPanel->updateInfo`.
2. Check `HasThumbnailRole` from `idx` to determine if `decData` is a default icon vs a real thumbnail, and pass `isDefaultIcon` to `metaPanel->setImagePreview`.

```
<<<<<<< SEARCH
            bool encrypted = idx.isValid() ? idx.data(EncryptedRole).toBool() : false;

            metaPanel->updateInfo(
                name, type, sizeStr, "-", mtimeStr, "-",
                path, encrypted, 0, 0
            );

            auto meta = MetadataManager::instance().getMeta(path.toStdWString());
=======
            bool encrypted = idx.isValid() ? idx.data(EncryptedRole).toBool() : false;

            auto meta = MetadataManager::instance().getMeta(path.toStdWString());

            metaPanel->updateInfo(
                name, type, sizeStr, "-", mtimeStr, "-",
                path, encrypted, meta.width, meta.height
            );
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
                QVariant decData = idx.data(Qt::DecorationRole);
                QPixmap previewPixmap;
                if (decData.canConvert<QIcon>()) {
                    previewPixmap = decData.value<QIcon>().pixmap(128, 128);
                } else if (decData.canConvert<QPixmap>()) {
                    previewPixmap = decData.value<QPixmap>();
                }
                metaPanel->setImagePreview(previewPixmap);
=======
                QVariant decData = idx.data(Qt::DecorationRole);
                bool hasThumb = idx.data(HasThumbnailRole).toBool();
                QPixmap previewPixmap;
                if (decData.canConvert<QIcon>()) {
                    previewPixmap = decData.value<QIcon>().pixmap(128, 128);
                } else if (decData.canConvert<QPixmap>()) {
                    previewPixmap = decData.value<QPixmap>();
                }
                metaPanel->setImagePreview(previewPixmap, !hasThumb);
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Clean and build the target:
   `cmake --build build --config Release`
2. Verify:
   - Select an image file (e.g. `.png` / `.jpg`). The "基础属性" section in `MetaPanel` should display the actual dimensions `W x H 像素`.
   - Select a non-image file or folder without thumbnail. The top preview should display a default icon restricted to at most 35x45 pixels without overflow or excessive scaling.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- **Metadata SSOT**: Directly reuses `MetadataManager::instance().getMeta(path)` to retrieve `meta.width` and `meta.height`.
- **Thumbnail SSOT**: Uses `HasThumbnailRole` (from `ModelContract.h`) to accurately distinguish real thumbnails from default icons.

---

## 6. Header API Signature Verification
- `MetaPanel::updateInfo(const QString&, const QString&, const QString&, const QString&, const QString&, const QString&, const QString&, bool, int width, int height)`: Checked in `MetaPanel.h`.
- `MetadataManager::getMeta(const std::wstring&)`: Returns `RuntimeMeta` struct containing `.width` and `.height`. Checked in `MetadataManager.h`.
- `QModelIndex::data(int role)`: Retrives `HasThumbnailRole` (`Qt::UserRole + 202`). Checked in `ModelContract.h`.

---

## 7. Header Inclusion Chain & Type Completeness Check
- `src/ui/PanelMediator.cpp` includes `../core/ModelContract.h`, ensuring `HasThumbnailRole` is fully declared.
- `src/ui/MetaPanel.h` includes `<QPixmap>`, `MetaPanel.cpp` includes `<QPainter>`, `<QPainterPath>`.
