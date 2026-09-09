# FramelessDialog Mouse Grab & Cursor Reset Fix Implementation Plan

## Overview
When frameless dialogs (such as `TagManagerDialog`) close, Qt implicit mouse grabs (`QApplication::mouseGrabber()`) and Win32 native mouse capture (`ReleaseCapture()`) can remain held on the closing dialog. This prevents `MainWindow` from receiving native `WM_NCHITTEST` messages, causing `MainWindow` to lose titlebar dragging, 8px edge resizing (no double-headed arrow cursors), and widget cursor updates.

This plan overrides `hideEvent` in `FramelessDialog` to explicitly release Win32 mouse capture (`::ReleaseCapture()`) and Qt mouse grab (`QApplication::mouseGrabber()->releaseMouse()`), ensuring `MainWindow` immediately regains full native window interactivity and correct cursor states upon dialog closure.

## Modified Files List
- `src/ui/FramelessDialog.cpp`

## Detailed Line-by-Line Changes

### `src/ui/FramelessDialog.cpp`
```diff
<<<<<<< SEARCH
void FramelessDialog::hideEvent(QHideEvent* event) {
    QGuiApplication::setOverrideCursor(Qt::ArrowCursor);
    QGuiApplication::restoreOverrideCursor();
    QDialog::hideEvent(event);
}
=======
void FramelessDialog::hideEvent(QHideEvent* event) {
#ifdef Q_OS_WIN
    ::ReleaseCapture();
#endif
    if (QWidget* grabber = QApplication::mouseGrabber()) {
        grabber->releaseMouse();
    }
    QGuiApplication::setOverrideCursor(Qt::ArrowCursor);
    QGuiApplication::restoreOverrideCursor();
    QDialog::hideEvent(event);
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Apply diff to `src/ui/FramelessDialog.cpp`.
2. Build:
   ```bash
   cmake -B build
   cmake --build build
   ```
3. Open and close `TagManagerDialog`, and verify `MainWindow` can immediately be moved, resized at edges with double-headed arrow cursors, and that the cursor shape updates correctly.
