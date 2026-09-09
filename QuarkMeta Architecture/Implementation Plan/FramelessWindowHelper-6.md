# FramelessWindowHelper HWND Native Dark Background Brush Implementation Plan

## 1. Overview
When a frameless window (`MainWindow`) is maximized, Windows DWM resizes the native HWND before Qt completes layout recalculation and widget repaint. The default Windows HWND class background brush (`GCLP_HBRBACKGROUND`) is pure black, causing a black background flash during the maximization transition.

This plan sets the native HWND class background brush in `FramelessWindowHelper.cpp` to match QuarkMeta's primary dark color (`#1E1E1E`, `RGB(0x1E, 0x1E, 0x1E)`). This ensures that any unrendered areas exposed during Win32 HWND maximization transitions match the app's dark theme color seamlessly, eliminating black background flashes.

## 2. Modified Files List
- `src/ui/FramelessWindowHelper.cpp`

## 3. Detailed Line-by-Line Changes

### `src/ui/FramelessWindowHelper.cpp`

```diff
<<<<<<< SEARCH
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
#endif
}
=======
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);

    // 🚀【原生暗色刷】：将 HWND 类默认背景刷设置为 #1E1E1E，消除 DWM 最大化瞬间未渲染区域的黑色硬黑块
    HBRUSH darkBrush = CreateSolidBrush(RGB(0x1E, 0x1E, 0x1E));
    SetClassLongPtrW(hwnd, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(darkBrush));
#endif
}
>>>>>>> REPLACE
```

## 4. Build & Verification Steps
1. Compile the application using CMake / MSVC build pipeline.
2. Launch QuarkMeta and click the maximize button or double-click the title bar.
3. Verify that the window maximization transition renders with a consistent `#1E1E1E` dark background without black flashing artifacts.
