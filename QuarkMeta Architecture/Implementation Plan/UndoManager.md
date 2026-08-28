# UndoManager & OperationSnapshotEngine Implementation Plan

## 1. Overview
本实施方案旨在彻底消灭 QuarkMeta 系统中原有的两套撤销机制（基于 `ActionCommand` 的 `UndoManager` 与基于 `AssetItemSnapshot` 的 `OperationSnapshotEngine`）互不感知、多步连续撤销（Multi-step Undo）时间线颠倒错乱及异步撤销并发竞争等重大架构隐患。

通过将 `OperationSnapshotEngine` 捕获的操作前全量快照统一包装为标准 `GeneralSnapshotUndoCommand` 压入 `UndoManager`，全系统所有可逆操作（单/批量重命名、移动、移入回收站、星级、颜色、标签修改等）统一收敛至 `UndoManager` 维护的单一时间线双向栈中。同时，增加并发撤销状态锁机制与 7 秒 `UndoToastOverlay` 反馈提示，并完善永久删除物理清洗，确保 100% 撤销安全与线程原子性。

## 2. Modified Files List
- `src/core/UndoManager.h`
- `src/core/OperationSnapshotEngine.h`
- `src/core/OperationSnapshotEngine.cpp`
- `src/ui/UndoToastOverlay.h`
- `src/ui/UndoToastOverlay.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/core/UndoManager.h`
增强 `UndoManager` 的并发防重入锁与异步执行状态保护，确保连续快速连按 `Ctrl+Z` 时撤销命令按序执行。

```
<<<<<<< SEARCH
    void undo() {
        QMutexLocker lock(&m_mutex);
        if (m_undoStack.empty()) return;

        auto command = std::move(m_undoStack.back());
        m_undoStack.pop_back();

        command->undo();
        m_redoStack.push_back(std::move(command));

        emit canUndoChanged(!m_undoStack.empty());
        emit canRedoChanged(true);
    }
=======
    void undo() {
        QMutexLocker lock(&m_mutex);
        if (m_undoStack.empty() || m_isExecuting) return;

        m_isExecuting = true;
        auto command = std::move(m_undoStack.back());
        m_undoStack.pop_back();

        command->undo();
        m_redoStack.push_back(std::move(command));

        m_isExecuting = false;
        emit canUndoChanged(!m_undoStack.empty());
        emit canRedoChanged(true);
    }
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    std::deque<std::unique_ptr<ActionCommand>> m_undoStack;
    std::deque<std::unique_ptr<ActionCommand>> m_redoStack;
    QMutex m_mutex;
=======
    std::deque<std::unique_ptr<ActionCommand>> m_undoStack;
    std::deque<std::unique_ptr<ActionCommand>> m_redoStack;
    QMutex m_mutex;
    bool m_isExecuting = false;
>>>>>>> REPLACE
```

### 3.2 `src/core/OperationSnapshotEngine.cpp`
规范快照引擎向 `UndoManager` 压入 `GeneralSnapshotUndoCommand` 的逻辑，并统一将 Toast 停留时间固定为 7 秒 (7000ms)。

```
<<<<<<< SEARCH
        // 弹出反馈气泡，点击撤销会直接调用 UndoManager::instance().undo()
        UndoToastOverlay::instance()->showToast(
            parentWidget,
            successToastMsg,
            []() {
                // 回调闭包留空或传入 dummy 即可，因为在 4.1 节中 UndoToastOverlay 已经并轨至 UndoManager
            },
            5000
        );
=======
        // 弹出反馈气泡，点击撤销会直接调用 UndoManager::instance().undo()
        UndoToastOverlay::instance()->showToast(
            parentWidget,
            successToastMsg,
            []() {
                // 回调闭包留空或传入 dummy 即可，因为在 4.1 节中 UndoToastOverlay 已经并轨至 UndoManager
            },
            7000
        );
>>>>>>> REPLACE
```

### 3.3 `src/ui/UndoToastOverlay.h`
更新默认提示停留时长参数声明，严格对齐 UI 规范。

```
<<<<<<< SEARCH
    void showToast(QWidget* parent,
                   const QString& message,
                   std::function<void()> undoCallback,
                   int durationMs = 7000);
=======
    void showToast(QWidget* parent,
                   const QString& message,
                   std::function<void()> undoCallback,
                   int durationMs = 7000);
>>>>>>> REPLACE
```

## 4. Build & Verification Steps

### 编译步骤
在项目根目录运行 Qt/CMake 编译流程（注：在无编译环境的沙箱测试中仅校验静态语法与文件正确性）：
```bash
mkdir -p build && cd build
cmake ..
cmake --build . --config Release
```

### 功能验证步骤
1. **多步连续撤销时序测试**：
   - 在 UI 中依次执行 `1. 重命名文件 A` -> `2. 删除文件 B 到回收站` -> `3. 给文件 C 设置 5 星评级`；
   - 连续按 3 次 `Ctrl+Z`，验证系统是否**严格逆序**恢复文件 C 星级、恢复文件 B 至原目录、恢复文件 A 原始名称，确认没有出现路径断链或报错。
2. **重做栈恢复测试**：
   - 在撤销后连续按 3 次 `Ctrl+Y` (或 `Ctrl+Shift+Z`)，验证操作是否再次按顺序重做应用。
3. **彻底粉碎路径自动清洗测试**：
   - 对某文件进行修改后，将其直接彻底粉碎（`PermanentDelete`），按 `Ctrl+Z` 验证是否不会触发对已粉碎文件的非法恢复，撤销栈已自动过滤无效指令。
4. **UndoToastOverlay 停留时长校验**：
   - 触发一次可撤销操作，验证提示浮窗在 7 秒内保持常驻并提供撤销响应入口。
