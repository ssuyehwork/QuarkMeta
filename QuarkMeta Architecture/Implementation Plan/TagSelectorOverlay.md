# TagSelectorOverlay Mouse Grab & Cursor Reset Fix Implementation Plan

## Overview
When `TagSelectorOverlay` closes (e.g., via clicking outside or tag selection), Qt's implicit mouse grab (`QApplication::mouseGrabber()`) and Win32 mouse capture (`ReleaseCapture()`) can remain locked on the overlay or its child widgets. This mouse capture lock prevents `MainWindow` from receiving native `WM_NCHITTEST` events, causing `MainWindow` to lose edge resizing (no double-headed arrow cursors), titlebar dragging, and cursor updates.

This plan explicitly releases Win32 mouse capture (`::ReleaseCapture()`) and Qt mouse grab (`QApplication::mouseGrabber()->releaseMouse()`) inside `closeOverlay()`, restoring native event routing and edge resizing to `MainWindow`.

## Modified Files List
- `src/ui/TagSelectorOverlay.cpp`

## Detailed Line-by-Line Changes

### `src/ui/TagSelectorOverlay.cpp`
```diff
<<<<<<< SEARCH
void TagSelectorOverlay::closeOverlay() {
    if (m_isClosing) return;
    m_isClosing = true;
    emit overlayClosed();
    close();
    QGuiApplication::setOverrideCursor(Qt::ArrowCursor);
    QGuiApplication::restoreOverrideCursor();
    deleteLater();
}
=======
void TagSelectorOverlay::closeOverlay() {
    if (m_isClosing) return;
    m_isClosing = true;
#ifdef Q_OS_WIN
    ::ReleaseCapture();
#endif
    if (QWidget* grabber = QApplication::mouseGrabber()) {
        grabber->releaseMouse();
    }
    emit overlayClosed();
    close();
    QGuiApplication::setOverrideCursor(Qt::ArrowCursor);
    QGuiApplication::restoreOverrideCursor();
    deleteLater();
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Apply the diff to `src/ui/TagSelectorOverlay.cpp`.
2. Build:
   ```bash
   cmake -B build
   cmake --build build
   ```
3. Test opening and closing `TagSelectorOverlay` and verify that `MainWindow` can be moved by titlebar, resized at edges with double-headed arrow cursors, and that the cursor resets immediately.
