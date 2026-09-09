# FramelessDialog Cursor Reset Fix Implementation Plan

## Overview
This plan fixes the persistent hand cursor bug where closing frameless dialogs (such as `TagManagerDialog`) leaves the mouse cursor stuck as a pointing hand (`Qt::PointingHandCursor`) over the main window until moved. By overriding `hideEvent` in `FramelessDialog` to trigger a temporary global cursor override cycle (`QGuiApplication::setOverrideCursor(Qt::ArrowCursor)` followed by `QGuiApplication::restoreOverrideCursor()`), Qt forces Windows and internal event loops to re-evaluate and apply the cursor for the widget directly under the cursor position (`QCursor::pos()`).

## Modified Files List
- `src/ui/FramelessDialogBase.h`
- `src/ui/FramelessDialog.cpp`

## Detailed Line-by-Line Changes

### `src/ui/FramelessDialogBase.h`
```diff
<<<<<<< SEARCH
    void setVisibleButtons(int flags);

protected:
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
=======
    void setVisibleButtons(int flags);

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
>>>>>>> REPLACE
```

### `src/ui/FramelessDialog.cpp`
```diff
<<<<<<< SEARCH
void FramelessDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
}
=======
void FramelessDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
}

void FramelessDialog::hideEvent(QHideEvent* event) {
    QGuiApplication::setOverrideCursor(Qt::ArrowCursor);
    QGuiApplication::restoreOverrideCursor();
    QDialog::hideEvent(event);
}
>>>>>>> REPLACE
```

## Build & Verification Steps
1. Apply diffs to `src/ui/FramelessDialogBase.h` and `src/ui/FramelessDialog.cpp`.
2. Build and verify:
   ```bash
   cmake -B build
   cmake --build build
   ```
3. Open and close `TagManagerDialog` and verify the cursor immediately resets to the active widget cursor without requiring manual mouse movement.
