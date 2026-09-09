# TagSelectorOverlay Cursor Reset Fix Implementation Plan

## Overview
This plan fixes the persistent hand cursor bug where closing `TagSelectorOverlay` leaves the mouse cursor stuck as a pointing hand (`Qt::PointingHandCursor`) over the main window until moved. By forcing a temporary global cursor override cycle (`QGuiApplication::setOverrideCursor(Qt::ArrowCursor)` followed by `QGuiApplication::restoreOverrideCursor()`) upon `closeOverlay()`, Qt forces Windows and internal event loops to re-evaluate and apply the cursor for the widget directly under the cursor position (`QCursor::pos()`).

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
    deleteLater();
}
=======
void TagSelectorOverlay::closeOverlay() {
    if (m_isClosing) return;
    m_isClosing = true;
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
2. Build and verify:
   ```bash
   cmake -B build
   cmake --build build
   ```
3. Test opening and closing `TagSelectorOverlay` and verify that the mouse cursor immediately resets to default arrow cursor without needing mouse movement.
