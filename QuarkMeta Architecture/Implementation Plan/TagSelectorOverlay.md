# TagSelectorOverlay Implementation Plan

## 1. Overview
本实施方案旨在彻底剥离 `TagSelectorOverlay`（标签选择弹出组件）在构造时向 `qApp` 全局 `QCoreApplication` 挂载事件过滤器（`qApp->installEventFilter(this)`）的违规设计。

通过取消全局事件过滤器，将其改造为原生的失焦/激活变更处理（`QEvent::ActivationChange` 与 `QEvent::WindowDeactivate`）及父控件局域事件监听，确保弹窗能够在其失去焦点或外部区域点击时优雅关闭，同时 100% 杜绝对应用中其他窗口、右键菜单和按键响应链路的全局污染。

## 2. Modified Files List
- `src/ui/TagSelectorOverlay.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/TagSelectorOverlay.cpp`
从构造函数与析构函数中彻底移除 `qApp` 事件过滤器的安装与卸载，并在 `changeEvent` 中使用原生的 `ActivationChange` 处理优雅关闭。

```
<<<<<<< SEARCH
    m_searchEdit->installEventFilter(this);
    m_tagGridWidget->installEventFilter(this);

    // 🚨 无论在任何时候任何情况下，一旦失去焦点或外部发生点击，立即关闭浮层
    qApp->installEventFilter(this);
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget* old, QWidget* now) {
        Q_UNUSED(old);
        if (!m_isClosing && isVisible() && now && now != this && !this->isAncestorOf(now)) {
            closeOverlay();
        }
    });
}

TagSelectorOverlay::~TagSelectorOverlay() {
    if (qApp) {
        qApp->removeEventFilter(this);
    }
}
=======
    m_searchEdit->installEventFilter(this);
    m_tagGridWidget->installEventFilter(this);

    if (parentWidget()) {
        parentWidget()->installEventFilter(this);
    }
}

TagSelectorOverlay::~TagSelectorOverlay() {
    if (parentWidget()) {
        parentWidget()->removeEventFilter(this);
    }
}
>>>>>>> REPLACE
```

```
<<<<<<< SEARCH
void TagSelectorOverlay::changeEvent(QEvent* event) {
    if (event->type() == QEvent::ActivationChange || event->type() == QEvent::WindowDeactivate) {
        if (!isActiveWindow() && !this->isAncestorOf(QApplication::focusWidget())) {
            closeOverlay();
        }
    }
    QFrame::changeEvent(event);
}
=======
void TagSelectorOverlay::changeEvent(QEvent* event) {
    if (event->type() == QEvent::ActivationChange || event->type() == QEvent::WindowDeactivate) {
        if (!isActiveWindow() && !this->isAncestorOf(QApplication::focusWidget())) {
            closeOverlay();
        }
    }
    QFrame::changeEvent(event);
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps

### 编译步骤
在项目根目录运行 CMake 编译命令（在测试验证环境下仅做代码与静态规范校验）：
```bash
mkdir -p build && cd build
cmake ..
cmake --build . --config Release
```

### 功能验证步骤
1. **失焦自动关闭测试**：
   - 在 UI 中呼出 `TagSelectorOverlay` 标签选择浮窗；
   - 点击主窗口其他区域或切换至其他应用程序，验证浮窗能否立刻优雅收起关闭，无残留窗口；
2. **全局事件干扰排查**：
   - 呼出浮窗后右键点击文件呼出上下文菜单，验证右键菜单正常弹出，且没有被 `TagSelectorOverlay` 误拦截；
3. **按键导航与 Esc 退出测试**：
   - 验证在浮层开启状态下，`Tab` 键切换焦点、方向键选择标签以及按 `Esc` 键退出浮窗的功能均维持 100% 正常。
