# NavPanel Implementation Plan

## 1. Overview
本实施方案旨在剥离 `NavPanel.cpp` 中的同步 `QDir` 物理目录树扫描逻辑，将侧边栏目录展开时的文件夹获取完全下沉至 `QtConcurrent` 后台工作线程，保障在展开深层或网络共享物理文件夹时主 UI 线程零卡顿。

## 2. Modified Files List
- `src/ui/NavPanel.h`
- `src/ui/NavPanel.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/NavPanel.h`
声明异步加载槽与挂起保护。

```
<<<<<<< SEARCH
    void expandPath(const QString& path);
=======
    void expandPath(const QString& path);
    void populateChildrenAsync(const QString& parentPath);
>>>>>>> REPLACE
```

### 3.2 `src/ui/NavPanel.cpp`
将同步 `QDir` 节点扫描替换为后台线程调度。

```
<<<<<<< SEARCH
    QDir dir(path);
=======
    // 异步后台加载子目录项
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
1. 点击侧边栏 NavPanel 的物理磁盘文件夹节点，验证展开层级流畅顺滑；
2. 验证路径选择信号正确广播并平滑更新 Central View。
