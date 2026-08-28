#include "DiskScanService.h" 
#include "FileFilterService.h" 
#include <QDir> 
#include <QFileInfo> 
 
namespace QuarkMeta { 
 
void DiskScanService::scanDirectoryChunked(const QString& path,
                                           bool recursive,
                                           std::function<void(std::vector<ItemRecord>&& chunk, bool isFirstChunk)> onChunkReady,
                                           const std::function<bool()>& shouldContinue) {
    std::vector<ItemRecord> currentChunk;
    bool isFirstChunk = true;
    const size_t chunkSize = 100;

    std::function<void(const QString&, bool)> scanDir;
    scanDir = [&](const QString& p, bool rec) {
        QDir dir(p);
        if (!dir.exists()) return;

        QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden, QDir::DirsFirst | QDir::Name);
        for (const QFileInfo& info : entries) {
            if (shouldContinue && !shouldContinue()) return;

            QString absPath = info.absoluteFilePath();
            if (FileFilterService::isAuxiliaryFile(absPath)) continue;

            ItemRecord itemRec = ItemRecord::create(absPath, nullptr);
            currentChunk.push_back(itemRec);

            if (currentChunk.size() >= chunkSize) {
                if (onChunkReady) {
                    onChunkReady(std::move(currentChunk), isFirstChunk);
                    isFirstChunk = false;
                }
                currentChunk.clear();
            }

            if (rec && info.isDir()) {
                scanDir(absPath, true);
            }
        }
    };

    scanDir(path, recursive);

    if (!currentChunk.empty() || isFirstChunk) {
        if (onChunkReady) {
            onChunkReady(std::move(currentChunk), isFirstChunk);
        }
    }
}

std::vector<ItemRecord> DiskScanService::scanDirectory(const QString& path, 
                                                        bool recursive, 
                                                        const std::function<bool()>& shouldContinue) { 
    std::vector<ItemRecord> allItems; 
 
    std::function<void(const QString&, bool)> scanDir; 
    scanDir = [&](const QString& p, bool rec) { 
        QDir dir(p); 
        if (!dir.exists()) return; 
 
        QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden, QDir::DirsFirst | QDir::Name); 
        for (const QFileInfo& info : entries) { 
            if (shouldContinue && !shouldContinue()) return; 
 
            QString absPath = info.absoluteFilePath(); 
             
            // 🚨 统一调用文件过滤服务（归一化处理所有辅助文件、.arc、.QuarkMeta） 
            if (FileFilterService::isAuxiliaryFile(absPath)) continue; 
 
            ItemRecord itemRec = ItemRecord::create(absPath, nullptr); 
            allItems.push_back(itemRec); 
 
            if (rec && info.isDir()) { 
                scanDir(absPath, true); 
            } 
        } 
    }; 
 
    scanDir(path, recursive); 
    return allItems; 
}

std::vector<ItemRecord> DiskScanService::scanDirectory(const QString& path, 
                                                        bool recursive, 
                                                        std::shared_ptr<CancellationToken> token) { 
    return scanDirectory(path, recursive, [token]() {
        return token ? !token->isCanceled() : true;
    });
}

} // namespace QuarkMeta
