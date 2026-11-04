#pragma once

#include <QMargins>
#include <QPoint>
#include <QRect>
#include <QSize>

namespace mirante::platform {

// On Windows, keeps each dialog on the screen of the window it belongs to.
//
// Qt centres a dialog on its parent window, then keeps it inside the screen it
// has on record for that window. That record can be stale: a window that was
// shown while Windows reported one screen, or whose screen went away and came
// back, can keep the wrong one. The dialog then opens on another screen. When
// the record differs from the screen that holds the window, the dialog is
// placed on that screen, centred on the window, before Qt would place it. On
// macOS and Linux nothing changes. See docs/ui.md#dialogs-on-windows.

// Watches every dialog the application shows. main.cpp calls it once, after
// the QApplication exists.
void installDialogScreens();

// The top-left corner of a dialog's frame, centred on its window's frame and
// kept inside `available`, the free area of the screen that holds the window.
// `size` is the dialog without its frame, `frame` the frame's width on each
// side. A dialog larger than `available` keeps its top-left corner inside.
QPoint centredOnWindow(const QRect &window, const QSize &size, const QMargins &frame, const QRect &available);

} // namespace mirante::platform
