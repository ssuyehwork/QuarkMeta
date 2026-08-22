# QuarkMeta `src` 源码目录僵尸、幽灵与无意义冗余代码排查报告 (Mark.md)

> **版本契约与架构定义**：当前应用已全面转向纯直接磁盘模式（Pure Disk Mode），仅允许维护 **`global.db`**（保存全局标签字典 `tag_groups`/`tag_group_items`、驱动器属性 `drive_metadata` 以及回收站条目 `disk_trash`）和物理目录下的 **`.QuarkMeta.json`**（保存物理文件的元数据，如 rating, color, tags, note, url, pinned 等）。任何在 `src/` 目录中涉及历史托管镜像库模式（Memory Mode）、镜像数据库 `metadata` 表、`system_stats` 表、胶囊 `.arc` 归档格式、多驱动器 `QuarkMeta_*.db` 数据库、Base36 / 128-bit FolderId 生成、以及已在 `CMakeLists.txt` 中被排除但仍存留在 `src/` 磁盘上的 C/C++ 源文件，均判定为僵尸（Zombie）、幽灵（Ghost）或无意义冗余代码。

---

## 1. 未参与 CMake 编译的 `src/` 游离/僵尸源文件

下述 25 个 C++ 源码文件（以及 39 个 libtiff 工具文件）保存在 `src/` 目录下，但在 `CMakeLists.txt` 的显式源文件列表 (`SOURCES`) 中已被完全移除，不参与项目构建，属于可以直接物理删除的**游离僵尸文件**。

### 1.1 核心与元数据层僵尸文件 (`src/core/`, `src/meta/`)
1. **`DiskIngestionService.h` / `DiskIngestionService.cpp`** (`src/core/`)
   - **原因**：历史导入吞吐服务，纯磁盘模式下直接扫描物理目录，不再需要入库吞吐管线。
2. **`SystemBootstrapper.h` / `SystemBootstrapper.cpp`** (`src/core/`)
   - **原因**：历史系统启动器，包含 IOCP 监听初始化日志与旧库加载逻辑，现已被 `main.cpp` 和 `CoreController` 替代。
3. **`IngestionProgressEngine.h` / `IngestionProgressEngine.cpp`** (`src/meta/`)
   - **原因**：历史吞吐进度引擎，关联旧版镜像库建索引逻辑。
4. **`MetadataDao.h` / `MetadataDao.cpp`** (`src/meta/`)
   - **原因**：历史 SQLite `metadata` 数据表 CRUD 接口，现元数据直接读写 `.QuarkMeta.json`，`metadata` 表已废弃。
5. **`PhysicalDataExtractor.h` / `PhysicalDataExtractor.cpp`** (`src/meta/`)
   - **原因**：历史物理特征抽取器，已合并入 `DiskMediaExtractor` / `MediaExtractorPipeline`。

### 1.2 视图与 UI 交互层僵尸文件 (`src/ui/`, `src/util/`)
6. **`GridResultView.h` / `GridResultView.cpp`** (`src/ui/`)
   - **原因**：旧版网格视图实现，已被 `JustifiedView` / `ContentPanel` 统一视图替换。
7. **`JustifiedResultView.h` / `JustifiedResultView.cpp`** (`src/ui/`)
   - **原因**：旧版瀑布流结果视图，已被 `JustifiedView.cpp` 替代。
8. **`ListResultView.h` / `ListResultView.cpp`** (`src/ui/`)
   - **原因**：旧版列表视图，已被统一模型视图替代。
9. **`ProgressDialog.h`** (`src/ui/`)
   - **原因**：旧版进度弹窗头文件，无对应 `.cpp` 实现。
10. **`ScanStats.h`** (`src/ui/`)
    - **原因**：旧版扫描统计结构头文件。
11. **`StyleLibrary.h`** (`src/ui/`)
    - **原因**：旧版样式库头文件。
12. **`SvgIcons.h`** (`src/ui/`)
    - **原因**：旧版 SVG 图标定义，现统一使用 `SvgIconRenderer`。
13. **`AppDirectoryInitializer.h`** (`src/util/`)
    - **原因**：旧版应用目录初始化器。
14. **`DiskIoService.h`** (`src/util/`)
    - **原因**：旧版磁盘 I/O 服务，已重构至 `FileOperationHelper` / `ShellHelper`。
15. **`SecureFileEraser.h`** (`src/util/`)
    - **原因**：旧版安全文件擦除器。

### 1.3 第三方库 libtiff 中的游离工具文件 (`src/third_party/libtiff/`)
16. **共 39 个 libtiff 工具/可执行程序源文件**：
    - 包括 `tiff2pdf.c`, `tiff2ps.c`, `tiffdump.c`, `tiffinfo.c`, `tiffcp.c`, `tiffsplit.c`, `tiffset.c`, `xtiff.c`, `mkg3states.c`, `tiff-bi.c`, `tiff-grayscale.c`, `tiff-palette.c`, `tiff-rgb.c` 等。
    - **原因**：QuarkMeta 仅需要 libtiff 核心解码库能力，这些命令行工具源文件未参与编译，无用。

---

## 2. `src/` 中 CMake 编译源文件内的“幽灵/残留”冗余代码

虽然以下文件包含在 `CMakeLists.txt` 中并参与编译，但其内部依然包含已废弃的旧数据表、旧逻辑分支与未调用的幽灵类。

### 2.1 未使用的幽灵头文件 / 幽灵类 (`src/meta/`)
1. **`DatabaseMigrator.h`**
   - **代码位置**：`src/meta/DatabaseMigrator.h` (Line 18-24)
   - **冗余逻辑**：包含 `"CREATE TABLE IF NOT EXISTS metadata (...)"` SQL 建表语句。在 `DatabaseManager.cpp` 中仅 `#include` 但**从未被实例化或调用**，属于幽灵类；其引用的 `metadata` 表在 `global.db` 中早已废弃。

### 2.2 已废弃数据库表 (`metadata` / `system_stats`) 相关幽灵 SQL
1. **`TagRepository.cpp`** (`src/meta/`)
   - **代码位置**：Line 169 (`SELECT value FROM system_stats...`), Line 195 (`INSERT OR REPLACE INTO system_stats...`)
   - **原因**：`system_stats` 表已被从 `DatabaseManager` 的建表 Schema 中彻底物理删除，此处 SQL 执行属于死逻辑。
2. **`MetadataManager.cpp` / `MetadataManager.h`** (`src/meta/`)
   - **代码位置**：
     - `MetadataManager.cpp`: Line 63-70 (`SELECT ... FROM metadata`), Line 888, 955 (`UPDATE metadata ...`), Line 1703, 1795 (`DELETE FROM metadata ...`), Line 2263 (`metadata_fts` 表 SQL)
   - **原因**：纯磁盘模式下元数据修改直接写 `.QuarkMeta.json`，写入 `metadata` 表的代码已被绕过，属于历史残留。

### 2.3 已废弃 128-bit FolderId / Base36 ID / 内存映射幽灵代码
1. **`MetadataDefs.h`** (`src/meta/`)
   - **代码位置**：Line 35 (`std::string folderId`), Line 70 (`std::string folderId`)
   - **原因**：纯磁盘模式直接使用绝对磁盘路径作为唯一标识，无需 `folderId`。
2. **`MetadataManager.cpp` / `MetadataManager.h`** (`src/meta/`)
   - **代码位置**：`MetadataManager.h` Line 86 (`generateFallbackFolderId`), Line 457 (`m_folderIdToPath`)
   - **原因**：纯磁盘模式无需维护 FolderId 到 Path 的反向内存映射表。
3. **`QuarkMetaJson.cpp`** (`src/meta/`)
   - **代码位置**：Line 229, 257, 289, 329 (`file_id_128`)
   - **原因**：JSON 中 `file_id_128` 仅为历史兼容字段，无实质业务含义。

### 2.4 已废弃 Capsule / Arc 胶囊归档模式遗留分支
1. **`BasicCommands.h`** (`src/core/`)
   - **代码位置**：Line 222, 226, 236, 245, 307 (`bool isCapsule`, `m_isCapsule`)
   - **原因**：胶囊重命名分支 `isCapsule` 永为 `false`。
2. **`MetadataManager.cpp`** (`src/meta/`)
   - **代码位置**：Line 469 (`DELETE FROM metadata WHERE is_folder = 1 AND path LIKE '%.arc'`), Line 523 (`getCapsuleThumbnailReadOnly`)
   - **原因**：`.arc` 胶囊逻辑已废弃。

---

## 3. 汇总表：`src/` 目录下可手动物理删除的僵尸源文件清单

| 序号 | 所在目录 | 建议直接删除的文件名 |
| :--- | :--- | :--- |
| 1 | `src/core/` | `DiskIngestionService.h`, `DiskIngestionService.cpp`, `SystemBootstrapper.h`, `SystemBootstrapper.cpp` |
| 2 | `src/meta/` | `IngestionProgressEngine.h`, `IngestionProgressEngine.cpp`, `MetadataDao.h`, `MetadataDao.cpp`, `PhysicalDataExtractor.h`, `PhysicalDataExtractor.cpp`, `DatabaseMigrator.h` |
| 3 | `src/ui/` | `GridResultView.h`, `GridResultView.cpp`, `JustifiedResultView.h`, `JustifiedResultView.cpp`, `ListResultView.h`, `ListResultView.cpp`, `ProgressDialog.h`, `ScanStats.h`, `StyleLibrary.h`, `SvgIcons.h` |
| 4 | `src/util/` | `AppDirectoryInitializer.h`, `DiskIoService.h`, `SecureFileEraser.h` |
