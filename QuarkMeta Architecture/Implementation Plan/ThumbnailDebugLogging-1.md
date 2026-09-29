# Implementation Plan - Thumbnail Diagnostic & Root Cause Trace Logging (`ThumbnailDebugLogging-1.md`)

## 1. Overview
This implementation plan adds detailed diagnostic logging tagged with `[THUMB_DIAG]` across the entire thumbnail pipeline (`DiskMediaExtractor`, `ThumbnailPipelineService`, `DiskItemModel`, and `FormatDecoders`).

The purpose of these log entries is to provide runtime answers to two specific questions:
1. **Whether a local disk thumbnail cache already exists**: Verify if `.png` files exist in system temporary storage (`QuarkMeta_Thumbnails`) and whether SHA256 path hashes match.
2. **Root cause of missing/unrendered thumbnails when cache or file exists**:
   - Check if `.QuarkMeta.json` intercepted extraction due to `thumbStatus == 1` (previously failed or timed out).
   - Check if `QImage::load` failed on an existing disk cache file (e.g. file corruption or access lock).
   - Check if task generation ID (`m_currentGeneration`) changed during asynchronous decoding, causing callbacks to be discarded.
   - Check if AI / EPS format decoding failed in specific decoder channels (embedded preview, XMP, PDF, Shell, or Ghostscript).

---

## 2. Modified Files List
- `src/util/DiskMediaExtractor.cpp`
- `src/util/ThumbnailPipelineService.cpp`
- `src/ui/models/DiskItemModel.cpp`
- `src/ui/FormatDecoders.cpp`

---

## 3. Detailed Line-by-Line Changes

### 3.1 `src/util/DiskMediaExtractor.cpp`

```
<<<<<<< SEARCH
QImage DiskMediaExtractor::getCapsuleThumbnailReadOnly(const QString& filePath) {
    QString diskCachePath = getDiskThumbCachePath(filePath);
    bool exists = QFile::exists(diskCachePath);
    if (exists) {
        QImage img;
        if (img.load(diskCachePath)) {
            qDebug() << "[THUMB_TRACE] ReadOnly Cache HIT:" << QFileInfo(filePath).fileName() << "CachePath:" << diskCachePath << "Size:" << img.size();
            return img;
        } else {
            qDebug() << "[THUMB_TRACE] ReadOnly Cache Corrupt/Failed load:" << QFileInfo(filePath).fileName() << "CachePath:" << diskCachePath;
        }
    } else {
        qDebug() << "[THUMB_TRACE] ReadOnly Cache MISS (file does not exist):" << QFileInfo(filePath).fileName() << "ExpectedCachePath:" << diskCachePath;
    }
    return QImage();
}
=======
QImage DiskMediaExtractor::getCapsuleThumbnailReadOnly(const QString& filePath) {
    QString diskCachePath = getDiskThumbCachePath(filePath);
    bool exists = QFile::exists(diskCachePath);
    QString fileName = QFileInfo(filePath).fileName();
    if (exists) {
        QImage img;
        if (img.load(diskCachePath)) {
            qDebug() << "[THUMB_DIAG] [1.DISK_CACHE_HIT] Found valid disk thumbnail cache for:" << fileName
                     << "| CachePath:" << diskCachePath << "| ImageSize:" << img.size();
            return img;
        } else {
            qDebug() << "[THUMB_DIAG] [1.DISK_CACHE_CORRUPT] Disk cache file EXISTS at:" << diskCachePath
                     << "for file:" << fileName << "BUT QImage::load() failed! File may be corrupted or locked.";
        }
    } else {
        qDebug() << "[THUMB_DIAG] [1.DISK_CACHE_MISS] Disk thumbnail cache does NOT exist for:" << fileName
                 << "| TargetCachePath:" << diskCachePath;
    }
    return QImage();
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 2. 失败标记拦截路径：若 .QuarkMeta.json 中被标记 thumb_status == 1，说明此前已提取失败，直接跳过二次解码
    {
        std::lock_guard<std::mutex> lock(s_jsonSaveMutex);
        QuarkMetaJson jsonCache(parentDir.toStdWString());
        jsonCache.load();
        const auto& cachedItems = jsonCache.items();
        std::wstring wFileName = fileName.toStdWString();
        auto it = cachedItems.find(wFileName);
        if (it != cachedItems.end() && it->second.thumbStatus == 1) {
            qDebug() << "[THUMB_TRACE] Intercepted by thumb_status == 1 (Previously Failed/Skipped):" << fileName;
            return res;
        }
    }
=======
    // 2. 失败标记拦截路径：若 .QuarkMeta.json 中被标记 thumb_status == 1，说明此前已提取失败，直接跳过二次解码
    {
        std::lock_guard<std::mutex> lock(s_jsonSaveMutex);
        QuarkMetaJson jsonCache(parentDir.toStdWString());
        jsonCache.load();
        const auto& cachedItems = jsonCache.items();
        std::wstring wFileName = fileName.toStdWString();
        auto it = cachedItems.find(wFileName);
        if (it != cachedItems.end() && it->second.thumbStatus == 1) {
            qDebug() << "[THUMB_DIAG] [2.INTERCEPTED_THUMB_STATUS_1] File:" << fileName
                     << "was INTERCEPTED because .QuarkMeta.json in" << parentDir
                     << "has thumbStatus == 1 (marked as failed/skipped previously). Re-decoding skipped.";
            return res;
        }
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    // 3. 解码路径：单次解码同时获取原始分辨率与 512px 缩略图
    qDebug() << "[THUMB_TRACE] Attempting single pass decode for:" << fileName;
    DecodedMediaResult dec = ImageDecoderFacade::decodeSinglePass(filePath, size, 0, token);
    if (dec.isValid) {
        res.originalSize = dec.originalSize;
        if (res.thumbnail512.isNull() && !dec.thumbnail512.isNull()) {
            bool saved = saveDiskThumbnail(filePath, dec.thumbnail512);
            qDebug() << "[THUMB_TRACE] SinglePass Decode Success & Saved to Disk Cache:" << fileName << "Saved:" << saved << "Size:" << dec.thumbnail512.size();
            res.thumbnail512 = dec.thumbnail512;
        }
        res.isValid = true;
=======
    // 3. 解码路径：单次解码同时获取原始分辨率与 512px 缩略图
    qDebug() << "[THUMB_DIAG] [3.DECODE_START] Single pass decoding started for:" << fileName << "| Size:" << size;
    DecodedMediaResult dec = ImageDecoderFacade::decodeSinglePass(filePath, size, 0, token);
    if (dec.isValid) {
        res.originalSize = dec.originalSize;
        if (res.thumbnail512.isNull() && !dec.thumbnail512.isNull()) {
            bool saved = saveDiskThumbnail(filePath, dec.thumbnail512);
            qDebug() << "[THUMB_DIAG] [3.DECODE_SUCCESS] Decoded thumbnail for:" << fileName
                     << "| ThumbSize:" << dec.thumbnail512.size() << "| SavedToDiskCache:" << saved;
            res.thumbnail512 = dec.thumbnail512;
        }
        res.isValid = true;
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    } else {
        // 4. 解码与现有缩略图缓存均失败：在非中途取消情况下入队定时合并落盘 thumb_status = 1
        if (!token || !token->isCanceled()) {
            DiskMediaExtractor::scheduleFailureMark(parentDir, fileName);
        }
    }
=======
    } else {
        // 4. 解码与现有缩略图缓存均失败：在非中途取消情况下入队定时合并落盘 thumb_status = 1
        if (!token || !token->isCanceled()) {
            qDebug() << "[THUMB_DIAG] [4.DECODE_FAILED] Single pass decoding FAILED for:" << fileName
                     << ". Scheduling thumbStatus = 1 failure mark in .QuarkMeta.json.";
            DiskMediaExtractor::scheduleFailureMark(parentDir, fileName);
        } else {
            qDebug() << "[THUMB_DIAG] [4.DECODE_CANCELLED] Single pass decoding CANCELLED for:" << fileName;
        }
    }
>>>>>>> REPLACE
```

---

### 3.2 `src/util/ThumbnailPipelineService.cpp`

```
<<<<<<< SEARCH
            QImage finalImg = DiskMediaExtractor::getCapsuleThumbnailReadOnly(path);
            if (finalImg.isNull()) {
                qDebug() << "[THUMB_TRACE] ReadOnly cache miss in pipeline, calling decodeImageToThumbnail for:" << path;
                finalImg = decodeImageToThumbnail(path, targetSize);
            } else {
                qDebug() << "[THUMB_TRACE] ReadOnly cache hit in pipeline for:" << path;
            }
=======
            QImage finalImg = DiskMediaExtractor::getCapsuleThumbnailReadOnly(path);
            if (finalImg.isNull()) {
                qDebug() << "[THUMB_DIAG] Pipeline ReadOnly miss for:" << QFileInfo(path).fileName() << ", triggering decodeImageToThumbnail.";
                finalImg = decodeImageToThumbnail(path, targetSize);
            } else {
                qDebug() << "[THUMB_DIAG] Pipeline ReadOnly HIT for:" << QFileInfo(path).fileName();
            }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
                QMetaObject::invokeMethod(qApp, [this, path, targetSize, finalImg, taskGen, onSingleLoaded]() {
                    if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                        return;
                    }

                    QPixmap pix = QPixmap::fromImage(finalImg);
                    if (!pix.isNull()) {
                        QString key = QString("%1@%2").arg(QDir::toNativeSeparators(path).toLower()).arg(targetSize);
                        {
                            QMutexLocker locker(&m_cacheMutex);
                            m_memoryCache.insert(key, new QPixmap(pix), 1);
                        }

                        if (onSingleLoaded) {
                            onSingleLoaded(path, pix);
                        }
                    }
                }, Qt::QueuedConnection);
=======
                QMetaObject::invokeMethod(qApp, [this, path, targetSize, finalImg, taskGen, onSingleLoaded]() {
                    if (m_currentGeneration.load(std::memory_order_relaxed) != taskGen) {
                        qDebug() << "[THUMB_DIAG] Generation mismatch on main-thread dispatch for:" << QFileInfo(path).fileName()
                                 << "| TaskGen:" << taskGen << "vs CurrentGen:" << m_currentGeneration.load(std::memory_order_relaxed);
                        return;
                    }

                    QPixmap pix = QPixmap::fromImage(finalImg);
                    if (!pix.isNull()) {
                        QString key = QString("%1@%2").arg(QDir::toNativeSeparators(path).toLower()).arg(targetSize);
                        {
                            QMutexLocker locker(&m_cacheMutex);
                            m_memoryCache.insert(key, new QPixmap(pix), 1);
                        }

                        if (onSingleLoaded) {
                            onSingleLoaded(path, pix);
                        }
                    } else {
                        qDebug() << "[THUMB_DIAG] QPixmap::fromImage returned NULL pixmap for:" << QFileInfo(path).fileName();
                    }
                }, Qt::QueuedConnection);
>>>>>>> REPLACE
```

---

### 3.3 `src/ui/models/DiskItemModel.cpp`

```
<<<<<<< SEARCH
        QString ext = rec.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext);
        if (rec.isDir || !isGraphic) {
            if (!rec.isDir) {
                qDebug() << "[THUMB_TRACE] Row" << r << "File:" << rec.filename << "is NOT a graphics file (ext:" << ext << "), skipping thumbnail load.";
            }
            continue;
        }

        QString path = rec.path;
        if (m_iconCache.contains(path)) {
            qDebug() << "[THUMB_TRACE] Row" << r << "File:" << rec.filename << "already in m_iconCache, skipping.";
            continue;
        }
        if (m_requestedPaths.contains(path)) {
            qDebug() << "[THUMB_TRACE] Row" << r << "File:" << rec.filename << "already in m_requestedPaths, skipping.";
            continue;
        }
=======
        QString ext = rec.suffix.toLower();
        bool isGraphic = UiHelper::isGraphicsFile(ext);
        if (rec.isDir || !isGraphic) {
            if (!rec.isDir) {
                qDebug() << "[THUMB_DIAG] Row" << r << "File:" << rec.filename << "is NOT recognized as graphics file (ext:" << ext << "), skipping.";
            }
            continue;
        }

        QString path = rec.path;
        if (m_iconCache.contains(path)) {
            qDebug() << "[THUMB_DIAG] Row" << r << "File:" << rec.filename << "already cached in m_iconCache, skipping request.";
            continue;
        }
        if (m_requestedPaths.contains(path)) {
            qDebug() << "[THUMB_DIAG] Row" << r << "File:" << rec.filename << "already in m_requestedPaths (in-flight), skipping duplicate request.";
            continue;
        }
>>>>>>> REPLACE
```

---

### 3.4 `src/ui/FormatDecoders.cpp`

```
<<<<<<< SEARCH
    // 通道 3：Ghostscript 矢量引擎
    QImage gsImg = renderGhostscriptSafely(filePath, targetSize, customTimeoutMs, token);
    if (!gsImg.isNull()) {
        return gsImg;
    }

    // 通道 4：Windows 原生系统 PDF 引擎
    QImage pdfRenderImg = renderPdfAiFirstPage(filePath, targetSize);
    if (!pdfRenderImg.isNull()) {
        return pdfRenderImg;
    }
=======
    // 通道 3：Ghostscript 矢量引擎
    QImage gsImg = renderGhostscriptSafely(filePath, targetSize, customTimeoutMs, token);
    if (!gsImg.isNull()) {
        qDebug() << "[THUMB_DIAG] [AI_DECODE] Channel 3 Ghostscript SUCCESS for:" << QFileInfo(filePath).fileName();
        return gsImg;
    }

    // 通道 4：Windows 原生系统 PDF 引擎
    QImage pdfRenderImg = renderPdfAiFirstPage(filePath, targetSize);
    if (!pdfRenderImg.isNull()) {
        qDebug() << "[THUMB_DIAG] [AI_DECODE] Channel 4 PDF Native Render SUCCESS for:" << QFileInfo(filePath).fileName();
        return pdfRenderImg;
    }
>>>>>>> REPLACE
```

---

## 4. Build & Verification Steps
1. Recompile project: `cmake --build build --config Debug`.
2. Open target folder containing `.ai` and `.eps` files in QuarkMeta.
3. Observe debug log console output filtering for `[THUMB_DIAG]`.
4. Match logged filenames against missing thumbnails:
   - If log shows `[2.INTERCEPTED_THUMB_STATUS_1]`: Indicates `.QuarkMeta.json` contains `thumbStatus == 1`.
   - If log shows `[1.DISK_CACHE_MISS]` and `[4.DECODE_FAILED]`: Indicates format decoder failed on all fallback channels.
   - If log shows `[1.DISK_CACHE_CORRUPT]`: Indicates local PNG thumbnail cache file exists but failed to load.

---

## 5. SSOT API Reuse & Anti-Redundancy Self-Check
- Uses existing `getCapsuleThumbnailReadOnly` and `getCapsuleExtractResult` entry points.
- Modifies no public header signatures (`.h` files remain completely untouched).

---

## 6. Header API Signature Verification
- `DiskMediaExtractor::getCapsuleThumbnailReadOnly(const QString&)` (`src/util/DiskMediaExtractor.h`)
- `DiskMediaExtractor::getCapsuleExtractResult(const QString&, int, std::shared_ptr<CancellationToken>)` (`src/util/DiskMediaExtractor.h`)
- `ThumbnailPipelineService::loadBatchAsync(const QStringList&, int, std::function<...>)` (`src/util/ThumbnailPipelineService.h`)
- `DiskItemModel::loadThumbnailsForRows(const QList<int>&)` (`src/ui/models/DiskItemModel.h`)

---

## 7. Header Inclusion Chain & Type Completeness Check
- All modified files incorporate `<QDebug>`, `<QFileInfo>`, and `<QDir>`. No header inclusion chain breakages introduced.
