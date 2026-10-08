# MetaPanel Implementation Plan (Iterative Version 4)

> **Note**: As per AGENTS.md Protocol 3.1 (Immutable Plan Files), this is an iterative update (`MetaPanel-3.md`) strictly enforcing the user requirement:
> - The top preview area (`m_topPreviewBox` / `m_lblImagePreview`) in `MetaPanel` MUST rigidly lock its width and height to **220x220px** (`setFixedSize(220, 220)`).

## 1. Overview
This implementation plan strictly enforces rigid 220x220px dimensions for `MetaPanel`'s top preview container (`m_topPreviewBox` and `m_lblImagePreview`):
1. **Rigid 220x220px Bounding Box**:
   - `m_lblImagePreview->setFixedSize(220, 220)`
   - `m_topPreviewBox->setFixedSize(220, 220)`
   - Images and icons drawn inside are proportionally fitted inside the fixed 220x220px viewport without altering the 220x220px container size.
2. **Dimension Display Restoration in Basic Attributes**:
   - `PanelMediator.cpp` passes real cached `meta.width` and `meta.height` values from `MetadataManager` to `metaPanel->updateInfo`, restoring `W x H 像素` display in `MetaPanel`.

---

## 2. Modified Files List
1. `src/ui/MetaPanel.cpp`
2. `src/ui/PanelMediator.cpp`

---

## 3. Detailed Line-by-Line Changes

### Change 1: `src/ui/MetaPanel.cpp` - Initialize Rigid 220x220 Dimensions
In `MetaPanel::initUi`, explicitly lock `m_topPreviewBox` and `m_lblImagePreview` to `220x220` pixels.

```
<<<<<<< SEARCH
    m_topPreviewBox = new QWidget(m_container);
    m_topPreviewBox->setObjectName("TopPreviewBox");
    // TopPreviewBox style in style.qss
    QVBoxLayout* previewLayout = new QVBoxLayout(m_topPreviewBox);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(6);

    m_lblImagePreview = new QLabel(m_topPreviewBox);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setObjectName("MetaImagePreview");
    // MetaImagePreview style in style.qss
    m_lblImagePreview->hide();
    previewLayout->addWidget(m_lblImagePreview, 0, Qt::AlignHCenter);
=======
    m_topPreviewBox = new QWidget(m_container);
    m_topPreviewBox->setObjectName("TopPreviewBox");
    m_topPreviewBox->setFixedSize(220, 220);
    // TopPreviewBox style in style.qss
    QVBoxLayout* previewLayout = new QVBoxLayout(m_topPreviewBox);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(0);

    m_lblImagePreview = new QLabel(m_topPreviewBox);
    m_lblImagePreview->setAlignment(Qt::AlignCenter);
    m_lblImagePreview->setFixedSize(220, 220);
    m_lblImagePreview->setObjectName("MetaImagePreview");
    // MetaImagePreview style in style.qss
    m_lblImagePreview->hide();
    previewLayout->addWidget(m_lblImagePreview, 0, Qt::AlignCenter);
>>>>>>> REPLACE
```

---

### Change 2: `src/ui/MetaPanel.cpp` - Rigid 220x220 Image Preview Rendering
Update `MetaPanel::setImagePreview` so the preview label maintains a fixed `220x220` canvas and draws content centered inside it.

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

        QImage roundedImg(scaled.size(), QImage::Format_ARGB32_Premultiplied);
        roundedImg.fill(Qt::transparent);
        {
            QPainter painter(&roundedImg);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            QPainterPath path;
            path.addRoundedRect(QRectF(0, 0, scaled.width(), scaled.height()), 4.0, 4.0);
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, scaled);
        }

        m_lblImagePreview->setPixmap(QPixmap::fromImage(roundedImg));
        m_lblImagePreview->setFixedSize(scaled.size());

        m_lblImagePreview->show();
        if (m_topPreviewBox) m_topPreviewBox->show();
    }
    m_adjustTimer->start();
}
=======
void MetaPanel::setImagePreview(const QPixmap& pixmap) {
    if (!m_lblImagePreview) return;
    if (pixmap.isNull()) {
        m_lblImagePreview->clear();
        m_lblImagePreview->hide();
        if (m_topPreviewBox) m_topPreviewBox->hide();
    } else {
        QSize canvasSize(220, 220);
        QPixmap canvas(canvasSize);
        canvas.fill(Qt::transparent);

        QPixmap scaled = (pixmap.width() > 220 || pixmap.height() > 220)
            ? pixmap.scaled(QSize(220, 220), Qt::KeepAspectRatio, Qt::SmoothTransformation)
            : pixmap;

        {
            QPainter painter(&canvas);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            int x = (220 - scaled.width()) / 2;
            int y = (220 - scaled.height()) / 2;

            QPainterPath path;
            path.addRoundedRect(QRectF(x, y, scaled.width(), scaled.height()), 4.0, 4.0);
            painter.setClipPath(path);
            painter.drawPixmap(x, y, scaled);
        }

        m_lblImagePreview->setPixmap(canvas);
        m_lblImagePreview->setFixedSize(canvasSize);

        m_lblImagePreview->show();
        if (m_topPreviewBox) {
            m_topPreviewBox->setFixedSize(canvasSize);
            m_topPreviewBox->show();
        }
    }
    m_adjustTimer->start();
}
>>>>>>> REPLACE
```

---

### Change 3: `src/ui/MetaPanel.cpp` - Lock Height in `adjustFlowHeights`
Prevent `adjustFlowHeights` from overriding the fixed 220x220px height of `m_topPreviewBox`.

```
<<<<<<< SEARCH
void MetaPanel::adjustFlowHeights() {
    if (m_topPreviewBox && m_paletteFlowLayout) {
        int contentH = m_paletteFlowLayout->heightForWidth(m_topPreviewBox->width());
        bool hasPreview = (m_lblImagePreview && !m_lblImagePreview->pixmap().isNull());
        bool hasPalette = (m_paletteFlowLayout->count() > 0);
        if (hasPreview || hasPalette) {
            m_topPreviewBox->show();
            int previewH = hasPreview ? m_lblImagePreview->pixmap().height() : 0;
            int totalSpacing = (hasPreview && hasPalette) ? 6 : 0;
            m_topPreviewBox->setFixedHeight(contentH + previewH + totalSpacing);
        } else {
            m_topPreviewBox->hide();
            m_topPreviewBox->setFixedHeight(0);
        }
        m_paletteFlowLayout->activate();
    }
=======
void MetaPanel::adjustFlowHeights() {
    if (m_topPreviewBox) {
        bool hasPreview = (m_lblImagePreview && !m_lblImagePreview->pixmap().isNull());
        if (hasPreview) {
            m_topPreviewBox->show();
            m_topPreviewBox->setFixedSize(220, 220);
        } else {
            m_topPreviewBox->hide();
            m_topPreviewBox->setFixedHeight(0);
        }
    }
>>>>>>> REPLACE
```

---

### Change 4: `src/ui/PanelMediator.cpp` - Pass Real Dimensions
In `PanelMediator.cpp`, fetch real `meta.width` and `meta.height` from `MetadataManager` and pass to `metaPanel->updateInfo`.

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

---

## 4. Build & Verification Steps
1. Clean and build the target:
   `cmake --build build --config Release`
2. Verify:
   - Select any item (image or file). The top preview bounding box is rigidly locked to **220x220px**.
   - Select an image file. Real dimensions (`W x H 像素`) are displayed in the "基础属性" section.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Directly reuses `MetadataManager::instance().getMeta(path)` for dimensions.
- Reuses `QPainter` center alignment for canvas rendering.

---

## 6. Header API Signature Verification
- `MetaPanel::updateInfo(..., int width, int height)`: Checked in `MetaPanel.h`.
- `MetadataManager::getMeta(...)`: Checked in `MetadataManager.h`.
