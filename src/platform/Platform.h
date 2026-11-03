#pragma once

#include <QIcon>
#include <QString>

// Code that differs between Windows, macOS and Linux lives in this folder.
// Launcher.h starts terminals and programs. AttachedDialogs.h
// keeps dialogs above their window on macOS. Foreground.h lets a later start
// bring the running instance to the front on Windows, and tells whether the app
// is in front. GlobalHotkey.h and Tray.h let the window hide and come back.
// LoginItem.h starts the app when the user signs in.

namespace mirante::platform {

// "windows", "macos" or "linux".
QString systemName();

// Sets the icon of the app's windows, and of the taskbar on Windows and Linux.
// On macOS it does nothing: the Dock shows the bundle's icon file, which is
// drawn on the macOS icon grid, and windows have no icon. See decision 0092.
void setAppIcon(const QIcon &icon);

// Has windows draw through the graphics API from the start, as they must once
// they hold a web view. Without it, Windows makes a window's native surface
// again when its first web view appears, which looks as if the window closed
// and opened again. Call it before QApplication is made. It does nothing on
// macOS and Linux, where it has not been tried, and keeps a value the user
// set in QT_WIDGETS_RHI.
void composeWindowsThroughTheGraphicsApi();

} // namespace mirante::platform
