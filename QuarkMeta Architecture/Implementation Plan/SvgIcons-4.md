# SvgIcons-4.md Implementation Plan: 解决 MSVC C2026 字符串字面量超长错误

## 1. 架构原则评估 (3 Questions Check)

### 1.1 单一真理源 (Single Source of Truth)
- **SVG 图标字典 SSOT**：`src/ui/SvgIcons.h` 中的 `SvgIcons::icons` (`QMap<QString, QString>`) 是全应用 SVG 字符串的 SSOT 注册中心。

### 1.2 封装完整性 (Encapsulation Integrity)
- 仅修改 `"quarkmeta"` 的 C++ 字符串字面量拼接形式，不改变最终生成的 QString 字符串内容与公共接口。

### 1.3 根因与表面现象分析 (Root Cause vs Surface Phenomenon)
- **现象**：MSVC 编译器报错 `错误 C2026 字符串太大，已截断尾部字符 G:\...\SvgIcons.h(510)`。
- **根因**：MSVC (Visual Studio C++) 编译器对单个 C++ 字符串字面量（String Literal，包括 Raw String `R"svg(...)svg"`）有 16,380 字节的硬性上限限制。从 `imagetracer.js` 生成的高高清 SVG 包含大量复杂的 `<path>` 节点，单个 raw string 字面量超过了 16KB 限制。
- **正解**：根据 C++ 语法标准（Phase 6），相邻的字符串字面量会被编译器自动无缝拼接为单个字符串。将超长的 Raw String 按节点段落拆分为 3~4 个小于 8KB 的独立 Raw String 子字面量邻接放置：
  ```cpp
  {"quarkmeta",
      R"svg(<svg ...)svg"
      R"svg(<path ...)svg"
      R"svg(<path ...)svg"
  },
  ```
  这样既彻底绕过了 MSVC C2026 编译器 16KB 限制，又保证编译期 100% 无缝拼接成完整原始 SVG，且零运行时性能开销。

---

## 2. 详细实现方案

将 `src/ui/SvgIcons.h` 中的 `"quarkmeta"` 键对应的超长 Raw String 拆分成多段小 Raw String 拼接。

---

## 3. 验证计划
- 确认 C++ Raw string 分段无语法错误，保证编译无 C2026 警告/错误。
