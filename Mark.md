# QuarkMeta 当前版本僵尸、幽灵与无意义冗余代码排查报告 (Mark.md)

> **版本契约与架构定义**：当前应用已全面转向纯直接磁盘模式（Pure Disk Mode），仅允许维护 **`global.db`**（保存全局标签字典 `tag_groups`/`tag_group_items`、驱动器属性 `drive_metadata` 以及回收站条目 `disk_trash`）和物理目录下的 **`.QuarkMeta.json`**（保存物理文件的元数据，如 rating, color, tags, note, url, pinned 等）。任何涉及历史托管镜像库模式（Memory Mode）、镜像数据库 `metadata` 表、`system_stats` 表、胶囊 `.arc` 归档格式、多驱动器 `QuarkMeta_*.db` 数据库、Base36 / 128-bit FolderId 生成、已在 `CMakeLists.txt` 中被排除但仍存留在磁盘上的 `src/` 文件，均判定为僵尸（Zombie）、幽灵（Ghost）或无意义冗余代码。

---

## 1. 未参与 CMake 编译的物理游离/僵尸源文件 (`src/` 目录下)

下述 26 个 C++ 源码文件（以及 39 个 libtiff 工具类文件）虽然物理保存在 `src/` 目录下，但已在 `CMakeLists.txt` 显式源文件列表 (`SOURCES`) 中被完全移除，不参与项目构建。它们属于彻底死掉的**游离僵尸文件**。

### 1.1 核心与元数据层僵尸文件 (`src/core/`, `src/meta/`)
1. **`src/core/DiskIngestionService.h` & `DiskIngestionService.cpp`**
   - **状态**：未参与 CMake 编译
   - **原因**：历史导入/吞吐服务，纯磁盘模式下直接扫描物理目录，不再需要入库吞吐管线。
2. **`src/core/SystemBootstrapper.h` & `SystemBootstrapper.cpp`**
   - **状态**：未参与 CMake 编译
   - **原因**：历史系统启动器，包含 IOCP 监听初始化日志与旧库加载逻辑，现已被 `main.cpp` 和 `CoreController` 直接替代。
3. **`src/meta/IngestionProgressEngine.h` & `IngestionProgressEngine.cpp`**
   - **状态**：未参与 CMake 编译
   - **原因**：历史吞吐进度引擎，关联旧版镜像库建索引逻辑。
4. **`src/meta/MetadataDao.h` & `MetadataDao.cpp`**
   - **状态**：未参与 CMake 编译
   - **原因**：历史 SQLite `metadata` 数据表 CRUD 接口，现元数据直接读写 `.QuarkMeta.json`，`metadata` 表已废弃。
5. **`src/meta/PhysicalDataExtractor.h` & `PhysicalDataExtractor.cpp`**
   - **状态**：未参与 CMake 编译
   - **原因**：历史物理特征抽取器，已合并入 `DiskMediaExtractor` / `MediaExtractorPipeline`。

### 1.2 视图与 UI 交互层僵尸文件 (`src/ui/`, `src/util/`)
6. **`src/ui/GridResultView.h` & `GridResultView.cpp`**
   - **状态**：未参与 CMake 编译
   - **原因**：旧版网格视图实现，已被 `JustifiedView` / `ContentPanel` 新统一视图替换。
7. **`src/ui/JustifiedResultView.h` & `JustifiedResultView.cpp`**
   - **状态**：未参与 CMake 编译
   - **原因**：旧版瀑布流结果视图，已被 `JustifiedView.cpp` 替代。
8. **`src/ui/ListResultView.h` & `ListResultView.cpp`**
   - **状态**：未参与 CMake 编译
   - **原因**：旧版列表视图，已被统一模型视图替代。
9. **`src/ui/ProgressDialog.h`**
   - **状态**：未参与 CMake 编译
   - **原因**：旧版进度弹窗头文件，无对应 `.cpp` 实现。
10. **`src/ui/ScanStats.h`**
    - **状态**：未参与 CMake 编译
    - **原因**：旧版扫描统计结构头文件。
11. **`src/ui/StyleLibrary.h`**
    - **状态**：未参与 CMake 编译
    - **原因**：旧版样式库头文件。
12. **`src/ui/SvgIcons.h`**
    - **状态**：未参与 CMake 编译
    - **原因**：旧版 SVG 图标定义，现统一使用 `SvgIconRenderer`。
13. **`src/util/AppDirectoryInitializer.h`**
    - **状态**：未参与 CMake 编译
    - **原因**：旧版应用目录初始化器。
14. **`src/util/DiskIoService.h`**
    - **状态**：未参与 CMake 编译
    - **原因**：旧版磁盘 I/O 服务，已重构至 `FileOperationHelper` / `ShellHelper`。
15. **`src/util/SecureFileEraser.h`**
    - **状态**：未参与 CMake 编译
    - **原因**：旧版安全文件擦除器。

### 1.3 第三方库 libtiff 中的游离工具文件 (`src/third_party/libtiff/`)
16. **共 39 个 libtiff 工具/可执行程序源文件**：
    - 包括 `tiff2pdf.c`, `tiff2ps.c`, `tiffdump.c`, `tiffinfo.c`, `tiffcp.c`, `tiffsplit.c`, `tiffset.c`, `xtiff.c`, `mkg3states.c`, `tiff-bi.c`, `tiff-grayscale.c`, `tiff-palette.c`, `tiff-rgb.c` 等。
    - **原因**：QuarkMeta 仅需要 libtiff 核心解码库能力（已在 `CMakeLists.txt` 的 `LIBTIFF_SOURCES` 中精简指定），这些命令行工具源文件未编译且无用。

---

## 2. CMake 编译源文件中的“幽灵/残留”冗余代码

虽然以下文件被包含在 `CMakeLists.txt` 中并参与编译，但其内部依然包含已废弃的旧数据表、旧逻辑分支和无意义函数调用。

### 2.1 已废弃数据库表 (`metadata` / `system_stats`) 相关幽灵代码
1. **`src/meta/DatabaseMigrator.h`**
   - **代码位置**：Line 18-24
   - **冗余逻辑**：类 `DatabaseMigrator::ensureActivated(sqlite3* db)` 中包含 `"CREATE TABLE IF NOT EXISTS metadata (...)"` SQL 建表语句。
   - **判定理由**：`DatabaseManager::loadDb` 架构已纯化，仅保留 `tag_groups` / `tag_group_items` / `disk_trash` / `drive_metadata` 表。`DatabaseMigrator` 在 `DatabaseManager.cpp` 中仅 `#include` 但**从未被实例化或调用**，属于幽灵类；其引用的 `metadata` 表在 `global.db` 中早已废弃。
2. **`src/meta/TagRepository.cpp`**
   - **代码位置**：Line 169 (`SELECT value FROM system_stats...`), Line 195 (`INSERT OR REPLACE INTO system_stats...`)
   - **冗余逻辑**：在 `checkAndMigrate()` 函数中，尝试向 SQLite 的 `system_stats` 表读取和写入迁移标记。
   - **判定理由**：`system_stats` 数据表已从 `DatabaseManager` 的建表 Schema 中彻底物理删除，此处 SQL 执行会静默报错或属于无效逻辑。
3. **`src/meta/MetadataManager.cpp` & `src/meta/MetadataManager.h`**
   - **代码位置**：
     - `MetadataManager.cpp`: Line 63-70 (`SELECT ... FROM metadata`, `INSERT OR REPLACE INTO metadata`), Line 888, 955 (`UPDATE metadata ...`), Line 1703, 1795 (`DELETE FROM metadata ...`), Line 2042, 2089 (`SELECT 1 FROM metadata WHERE folder_id = ?`), Line 2263 (`metadata_fts` 全文检索表 SQL)
   - **冗余逻辑**：保留了大段对 SQLite `metadata` 表和 `metadata_fts` 全文检索表进行 CRUD 操作的 SQL 拼接与 Bind 代码。
   - **判定理由**：当前纯磁盘模式下，`MetadataManager` 更新属性（Rating, Color, Tags, Note, URL, Pinned）直接调用 `QuarkMetaJson::updateItemMeta` 写入物理 `.QuarkMeta.json` 文件。写入 `metadata` 表的代码已经被注释分支绕过或失效，属于历史残留。

### 2.2 已废弃 128-bit FolderId / Base36 ID / 内存映射幽灵代码
1. **`src/meta/MetadataDefs.h`**
   - **代码位置**：Line 35 (`std::string folderId`), Line 70 (`std::string folderId`), Line 48, 100
   - **冗余字段**：`ItemMetadata` 和 `ItemRecord` 结构体中仍包含 `folderId` 字段及其空值判断。
   - **判定理由**：Base36 / 128-bit Folder ID 是历史镜像数据库关联逻辑的产物。纯磁盘模式下直接使用绝对磁盘路径（`std::wstring path`）作为唯一标识。
2. **`src/meta/MetadataManager.cpp` & `src/meta/MetadataManager.h`**
   - **代码位置**：
     - `MetadataManager.h`: Line 86 (`generateFallbackFolderId`), Line 457 (`m_folderIdToPath`)
     - `MetadataManager.cpp`: Line 122 (`generateFallbackFolderId`), Line 322, 711, 781 (`m_folderIdToPath`)
   - **冗余逻辑**：`m_folderIdToPath` 映射表及 `generateFallbackFolderId` 物理 FRN 降级逻辑。
   - **判定理由**：纯磁盘模式无需维护 FolderId 到 Path 的反向内存映射表。
3. **`src/meta/QuarkMetaJson.cpp`**
   - **代码位置**：Line 229, 257, 289, 329
   - **冗余逻辑**：读写 JSON 配置文件中的 `"file_id_128"` 字段。
   - **判定理由**：`.QuarkMeta.json` 中物理存储文件元数据，`file_id_128` 仅为历史兼容字段，无实质业务含义。

### 2.3 已废弃 Capsule / Arc 胶囊归档模式遗留分支
1. **`src/core/BasicCommands.h`**
   - **代码位置**：Line 222, 226, 236, 245, 307, 316, 376, 397
   - **冗余逻辑**：`BatchRenameCommand` 构造函数入参 `bool isCapsule`、成员变量 `m_isCapsule` 以及 `if (isCapsule)` 条件判断分支。
   - **判定理由**：Capsule (`.arc`) 胶囊归档模式与 `CapsuleMediaExtractor` 已在前期版本清理中彻底物理抹除。重命名操作仅存在纯磁盘重命名模式，`isCapsule` 分支永为 `false`。
2. **`src/meta/MetadataManager.cpp`**
   - **代码位置**：Line 469 (`DELETE FROM metadata WHERE is_folder = 1 AND path LIKE '%.arc'`), Line 523 (`getCapsuleThumbnailReadOnly`)
   - **冗余逻辑**：清理 `.arc` 逻辑和读取胶囊缩略图调用。
   - **判定理由**：`.arc` 文件逻辑已废弃。

---

## 3. 根目录下残留的历史工程资产与冗余目录

在项目根目录下，存在大量历史版本备份、过程文档与设计原件。这些目录和文件不属于项目的运行时代码，但显著增加了仓库体积与干扰：

1. **历史版本备份目录**（应清理或移出代码库）：
   - `Dual‑mode version/` (双模式历史备份)
   - `Version-1/`, `Version-2/`, `Version-3/`, `Version-4/` (历史演进版本备份)
   - `初始版/` (最早初始版本)
   - `QuarkMeta Architecture/` (旧架构备份)
   - `RapidNotes/` (开发临时笔记)
   - `程序崩溃日志/` (历史运行崩溃 Dump/Log)
2. **废弃说明与调试文档**：
   - `Memory Mode.md` (已废弃的内存/镜像库模式说明)
   - `Memories.md` (历史记忆文档)
   - `ARCHITECTURE_DEBT.md` (旧架构债务清单)
   - `debug.md` (临时调试日志)
3. **设计源文件/大文件资产**：
   - `arcmeta.psd` (2.07 MB 设计图)
   - `arcmeta.psb` (638 KB 设计图)
   - `arcmeta.png` (47.9 KB)

---

## 4. 总结与量化指标

| 类别 | 统计数量 | 主要涉及文件 / 目录 | 说明 |
| :--- | :--- | :--- | :--- |
| **未编译僵尸源文件** | **26 个 C++ 文件 + 39 个 libtiff 工具文件** | `src/core/`, `src/meta/`, `src/ui/`, `src/util/` | 存在于 `src/` 中但在 `CMakeLists.txt` 中已被排除 |
| **幽灵类与废弃头文件** | **1 个** | `src/meta/DatabaseMigrator.h` | 仅 `#include`，未实例化调用，包含废弃建表 SQL |
| **废弃 DB 数据表残余** | **3 个核心文件** | `MetadataManager.cpp`, `TagRepository.cpp`, `DatabaseMigrator.h` | 包含 `metadata`, `system_stats`, `metadata_fts` SQL |
| **废弃 ID 机制与字段** | **3 个核心文件** | `MetadataDefs.h`, `MetadataManager.cpp`, `QuarkMetaJson.cpp` | `folderId`, `m_folderIdToPath`, `file_id_128` |
| **废弃 Capsule/Arc 分支**| **2 个核心文件** | `BasicCommands.h`, `MetadataManager.cpp` | `isCapsule`, `.arc` 清理逻辑 |
| **项目根目录历史冗余** | **9 个目录 + 7 个文档/资产文件** | `Version-*`, `Memory Mode.md`, `arcmeta.psd` 等 | 历史备份、过程文档与设计原件 |

---

> **结论**：上述标记的所有代码与文件均已精确定位。应用当前仅依赖 **`global.db`** 和 **`.QuarkMeta.json`**。清理建议为：后续重构中将未编译僵尸文件彻底从 `src/` 删除，清理 CMake 编译代码中的 `metadata`/`system_stats` 残留 SQL、`folderId` 内存表与 `isCapsule` 假分支，使 codebase 达成 100% 纯净。
