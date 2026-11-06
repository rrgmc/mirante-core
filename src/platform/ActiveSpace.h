#pragma once

#include <QList>

class QWidget;

namespace mirante::platform {

// On macOS, makes the given windows appear on the current desktop (Space)
// the next time they are shown, instead of on the desktop they were on when
// they were hidden. The windows keep the normal behaviour once they are on
// screen. Call it before showing them. On Windows and Linux it does nothing.
// See docs/ui.md#tray-and-notifications.
void showOnActiveSpace(const QList<QWidget *> &windows);

} // namespace mirante::platform
