# FilterPanel Implementation Plan

## 1. Overview
本实施方案旨在对 `FilterPanel`（筛选面板）进行物理解耦与瘦身重构。通过将内部混合的筛选状态计算、分类统计逻辑以及控件样式配置抽离，强化 `FilterStateModel` 与 `ScanStatsEngine` 的单一持有者地位，使 `FilterPanel` 保持极致纯洁的 UI 呈现层角色。

## 2. Modified Files List
- `src/ui/FilterPanel.h`
- `src/ui/FilterPanel.cpp`

## 3. Detailed Line-by-Line Changes

### 3.1 `src/ui/FilterPanel.h`
规范 `FilterPanel` 对外状态更新接口与只读访问方法。

```
<<<<<<< SEARCH
    void applyFilterState(const FilterState& state);
=======
    void applyFilterState(const FilterState& state);
    void resetAllFilters();
>>>>>>> REPLACE
```

### 3.2 `src/ui/FilterPanel.cpp`
收拢控件连接与数据状态解耦逻辑。

```
<<<<<<< SEARCH
    emit filterChanged(m_currentState);
=======
    emit filterChanged(m_currentState);
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
1. 点击 FilterPanel 筛选维度（如评级、色标、标签存在性等），验证右侧视图数据联动 100% 正确；
2. 点击“重置”按钮，验证所有筛选状态恢复默认并同步广播通知。
