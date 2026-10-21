# QuarkMeta 唯一真理源 (SSOT) 接口与命令字典

本文档记录全软件内部已完成归一化重构的唯一真理源 (SSOT) 标准业务入口与核心数据接口。全软件任何模块在触发对应业务时，必须且只能使用本文档记录的标准接口，严禁绕过核心总线或另起炉灶。

---

## 1. 核心命令总线 (CoreEngine AppCommand SSOT)

所有对磁盘文件元数据（评级、色标、标签、备注、URL、置顶等）的变更操作，统一使用 `CoreEngine::instance().executeCommand(cmd)` 作为唯一合法入口。

| 业务动作 | 对应 AppCommandType | 参数规格 (`cmd.params`) | 归一化入口调用方式 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| **星级设定 (Set Rating)** | `AppCommandType::SetRating` | `params["rating"] = int (0~5)` | `CoreEngine::instance().executeCommand(cmd)` | 自动兼容单选与多选集合，统一更新 SQLite 数据库与磁盘 `.QuarkMeta.json`，并触发 `CentralEventHub` 广播与 UI/FilterPanel 刷新。 |
| **手动色标标记 (Set Color)** | `AppCommandType::SetColor` | `params["color"] = QString (Hex色值)` | `CoreEngine::instance().executeCommand(cmd)` | 支持 `Alt + 1~9` 快捷键、右键菜单与 MetaPanel 标色归一化分发，更新数据库/侧车文件并广播变动。 |
| **标签管理 (Set/Add/Remove Tags)** | `AppCommandType::SetTags`<br>`AppCommandType::AddTag`<br>`AppCommandType::RemoveTag` | `params["tags"] = QStringList`<br>`params["tag"] = QString` | `CoreEngine::instance().executeCommand(cmd)` | 优先登记至 `TagLexiconService` 全局词库，再绑定落盘至各文件元数据。 |
| **备注与链接编辑 (Set Note / URL)** | `AppCommandType::SetNote`<br>`AppCommandType::SetURL` | `params["note"] = QString`<br>`params["url"] = QString` | `CoreEngine::instance().executeCommand(cmd)` | MetaPanel 等编辑失焦时统一同步落盘。 |

---

## 2. 数据访问层 (DAO SSOT)

| 数据访问动作 | 所属 DAO / 服务 | 物理精确签名 | 核心职责与性能优化规格 |
| :--- | :--- | :--- | :--- |
| **按路径列表批量查询** | `LibraryItemDao` | `static std::vector<ItemRecord> getItemsByPaths(const QStringList& paths);` | 以 500 个路径为一束（Chunking = 500），通过动态 SQL `IN (?, ...)` 批量查询 `library_items` 主表、`file_tags` 标签表及 `item_palettes` 色板表，消除 N+1 查询黑洞。 |
| **智能文件夹匹配路径查询** | `SmartFolderDao` | `static QStringList getMatchingPaths(const SmartFolderRecord& rec);` | 精准评估智能文件夹 SQL 规则（含 Rating, Color, Tag, Extension, Keyword 等），自动过滤 `sync_state != 2` 未删除状态，返回精准匹配文件路径集。 |
| **全库卡片记录提取** | `LibraryDao` | `static std::vector<ItemRecord> getCategoryIndexedItems(int categoryId);` | 允许 `categoryId == -1` 查询全库已索引条目。 |

---

## 3. UI 视图与交互联动 (UI Control SSOT)

| 交互/导航动作 | 承载组件 / 服务 | 标准调用 API / 信号 | 规则与契约 |
| :--- | :--- | :--- | :--- |
| **智能文件夹节点选中** | `LibraryPanel` ➔ `PanelMediator` | `connect(libraryPanel, &LibraryPanel::smartFolderSelected, ...)` | 通过 `SmartFolderDao::getMatchingPaths(record)` 仅加载匹配路径集至 `ContentPanel`，并调用 `filterPanel->clearAllFilters(false)` 重置侧边栏筛选。 |
| **按压星级改分判定** | `CardPainterHelper` | `CardPainterHelper::canModifyRatingOnPress(isSelected, modifiers)` | 仅当卡片已选中且未按住 Ctrl/Shift 组合键时允许直接点击星级修改，否则放行给视图原生选中逻辑。 |
| **筛选器重置** | `FilterPanel` | `void clearAllFilters(bool force = false);` | 增量重置筛选 UI 与代理模型状态，保留固定（Pinned）筛选状态。 |
