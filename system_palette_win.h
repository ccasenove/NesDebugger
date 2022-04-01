#ifndef _SYSTEM_PALETTE_WIN_H_
#define _SYSTEM_PALETTE_WIN_H_

#include <gtk/gtk.h>

#include "debugger_app.h"

#define SYSTEM_PALETTE_WINDOW_TYPE (system_palette_window_get_type())
G_DECLARE_FINAL_TYPE(SystemPaletteWindow, system_palette_window, SYSTEM_PALETTE, WINDOW, GtkWindow);

SystemPaletteWindow *system_palette_window_new(DebuggerApp *app);

#endif