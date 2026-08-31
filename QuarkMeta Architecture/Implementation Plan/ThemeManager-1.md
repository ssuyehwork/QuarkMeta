# QuarkMeta 架构重构方案：ThemeManager 五栏卡片间距与圆角复原

## 一、 Overview
将软件主界面五大独立栏区（NavPanel、FavoritePanel、ContentPanel、MetaPanel、FilterPanel）彻底重塑为独立精美卡片面板。通过给 `bodyLayout` 设置 5 像素外边距与内间隙，并在 QSS 中为各栏区容器添加 `1px solid #333333` 边框与 `4px` 圆角，实现栏区之间处处保持 5 像素物理隔离间隙。

## 二、 Modified Files List
- `src/ui/MainWindow.cpp`
- `src/ui/ThemeManager.cpp`

## 三、 Detailed Line-by-Line Changes

### 1. `src/ui/MainWindow.cpp`
```git
<<<<<<< SEARCH
    m_bodyLayout->setContentsMargins(0, 0, 0, 0);
    m_bodyLayout->setSpacing(0);
=======
    m_bodyLayout->setContentsMargins(5, 5, 5, 5);
    m_bodyLayout->setSpacing(5);
>>>>>>> REPLACE
```

### 2. `src/ui/ThemeManager.cpp`
```git
<<<<<<< SEARCH
        #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
            background-color: #1E1E1E; border: none; border-radius: 0px; margin: 0px; padding: 0px;
        }
        #ContainerHeader { background-color: #252526; border-bottom: 1px solid #333333; }
=======
        #SidebarContainer, #FavoriteContainer, #EditorContainer, #MetadataContainer, #FilterContainer {
            background-color: #1E1E1E;
            border: 1px solid #333333;
            border-radius: 4px;
        }
        #ContainerHeader {
            background-color: #252526;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            border-bottom: 1px solid #333333;
        }
>>>>>>> REPLACE
```

## 四、 Build & Verification Steps
1. 确认 `src/ui/MainWindow.cpp` 与 `src/ui/ThemeManager.cpp` 已修改。
2. 确认五大面板独立卡片质感，拥有 1px 细边框、4px 圆角与 5px 隔离间隙。
