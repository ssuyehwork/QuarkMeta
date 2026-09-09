# TagSelectorOverlay Native Event & Mouse Capture Release Implementation Plan

## Overview
Applying `FramelessWindowHelper` to `TagSelectorOverlay` (a `QFrame` floating overlay owned by `MainWindow`) modified its Win32 window style (`GWL_STYLE`) with `WS_THICKFRAME` and `WS_CAPTION`. When `TagSelectorOverlay` closed, Windows DWM and Win32 window management corrupted the non-client message routing (`WM_NCHITTEST`) for the owner window (`MainWindow`), preventing `MainWindow` from handling edge resizing (double-headed arrow cursors), titlebar dragging, and cursor updates.

This plan removes `FramelessWindowHelper` from `TagSelectorOverlay` (relying on its built-in Qt `mousePressEvent` / `mouseMoveEvent` dragging), and explicitly releases Win32 mouse capture (`ReleaseCapture()`) and Qt mouse grabs (`QWidget::mouseGrabber()`) upon `closeOverlay()`.

## Modified Files List
- `src/ui/TagSelectorOverlay.h`
- `src/ui/TagSelectorOverlay.cpp`

## Detailed Line-by-Line Changes

### `src/ui/TagSelectorOverlay.h`
```diff
<<<<<<< SEARCH
#include "components/FlowLayout.h"
#include "../core/TagLexiconService.h"
#include "FramelessWindowHelper.h"

namespace QuarkMeta {
=======
#include "components/FlowLayout.h"
#include "../core/TagLexiconService.h"

namespace QuarkMeta {
>>>>>>> REPLACE
<<<<<<< SEARCH
    bool m_isClosing = false;
    bool m_isDragging = false;
    QPoint m_dragPos;

    FramelessWindowHelper* m_framelessHelper = nullptr;
};
=======
    bool m_isClosing = false;
    bool m_isDragging = false;
    QPoint m_dragPos;
};
>>>>>>> REPLACE
```

### `src/ui/TagSelectorOverlay.cpp`
```diff
<<<<<<< SEARCH
    m_framelessHelper = FramelessWindowHelper::apply(this, nullptr);

    initUi();
=======
    initUi();
>>>>>>> REPLACE
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
#ifdef Q_OS_WIN
    ::ReleaseCapture();
#endif
    if (QWidget* grabber = QWidget::mouseGrabber()) {
        grabber->releaseMouse();
    }
    emit overlayClosed();
    close();
    deleteLater();
}
>>>>>>> REPLACE
<<<<<<< SEARCH
bool TagSelectorOverlay::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    if (m_framelessHelper && m_framelessHelper->handleNativeEvent(message, result)) {
        return true;
    }
    return QFrame::nativeEvent(eventType, message, result);
}
=======
bool TagSelectorOverlay::nativeEvent(const QByteArray& eventType, void* message, qintptr* result) {
    return QFrame::nativeEvent(eventType, message, result);
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Apply diffs to `src/ui/TagSelectorOverlay.h` and `src/ui/TagSelectorOverlay.cpp`.
2. Verify that `TagSelectorOverlay` closes cleanly and `MainWindow` retains titlebar dragging, edge resizing, and dynamic cursor updates.
