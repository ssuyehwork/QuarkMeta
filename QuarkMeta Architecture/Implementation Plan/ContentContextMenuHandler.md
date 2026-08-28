# ContentContextMenuHandler Implementation Plan

## 1. Overview
本实施方案旨在彻底拆解 `ContentPanel.cpp` 中膨胀的右键上下文菜单逻辑（第 1200~1450 行，超过 250 行）。通过创建独立的控制器 `ContentContextMenuHandler`（位于 `src/ui/handlers/`），将右键菜单 Action 的构建、Icon/Style 渲染、子菜单层级组装与选中 Action 响应全面剥离，使 `ContentPanel` 瘦身并实现高内聚解耦。

## 2. Modified Files List
- `src/ui/handlers/ContentContextMenuHandler.h` (新建)
- `src/ui/handlers/ContentContextMenuHandler.cpp` (新建)
- `src/ui/ContentPanel.h`
- `src/ui/ContentPanel.cpp`
- `CMakeLists.txt`

## 3. Detailed Line-by-Line Changes

### 3.1 `CMakeLists.txt`
注册新建的 handler 文件。

```
<<<<<<< SEARCH
    src/ui/AppShortcutController.cpp
=======
    src/ui/AppShortcutController.cpp
    src/ui/handlers/ContentContextMenuHandler.cpp
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
    src/ui/AppShortcutController.h
=======
    src/ui/AppShortcutController.h
    src/ui/handlers/ContentContextMenuHandler.h
>>>>>>> REPLACE
```

### 3.2 `src/ui/handlers/ContentContextMenuHandler.h`
创建独立的右键菜单处理器。

```cpp
#pragma once

#include <QObject>
#include <QPoint>
#include <QAbstractItemView>

namespace QuarkMeta {

class ContentPanel;

class ContentContextMenuHandler : public QObject {
    Q_OBJECT
public:
    explicit ContentContextMenuHandler(ContentPanel* panel);

    void showContextMenu(QAbstractItemView* view, const QPoint& pos);

private:
    ContentPanel* m_panel = nullptr;
};

} // namespace QuarkMeta
```

### 3.3 `src/ui/handlers/ContentContextMenuHandler.cpp`
收揽原 `ContentPanel::onCustomContextMenuRequested` 中的全部 QMenu 构建与 Action 路由逻辑。

```cpp
#include "ContentContextMenuHandler.h"
#include "../ContentPanel.h"
// 接入 UiHelper, ShellHelper, MetadataManager 等标准服务...
```

### 3.4 `src/ui/ContentPanel.cpp`
将庞大的右键菜单内联逻辑精简为单行 handler 调用。

```
<<<<<<< SEARCH
    // 🚀 【补丁彻底根除】：废除硬锁信号与物理禁用绘制！
    // 菜单弹出期间开启无锁模态标记，后台异步提取数据仅挂起不触发死锁，菜单关闭后自动 Flush
    m_isContextMenuActive = true;
    QAction* selectedAction = menu.exec(view->viewport()->mapToGlobal(pos));
    m_isContextMenuActive = false;
=======
    if (m_contextMenuHandler) {
        m_contextMenuHandler->showContextMenu(view, pos);
    }
>>>>>>> REPLACE
```

## 4. Build & Verification Steps

### 编译步骤
```bash
mkdir -p build && cd build
cmake ..
cmake --build . --config Release
```

### 验证步骤
1. 在文件列表区右键弹出菜单，验证常规操作（打开、在资源管理器中显示、复制、剪切、粘贴、刷新、重命名等）功能 100% 正常；
2. 在空白处右键弹出菜单，验证新建文件夹/文档与排序子菜单正确呈现并有效响应；
3. 对比 `ContentPanel.cpp`，代码行数精简 250+ 行。
