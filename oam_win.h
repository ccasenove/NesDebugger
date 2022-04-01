#ifndef _OAM_WIN_H_
#define _OAM_WIN_H_

#include <gtk/gtk.h>

#include "debugger_app.h"

#define OAM_WINDOW_TYPE (oam_window_get_type())
G_DECLARE_FINAL_TYPE(ObjectAttributeMemoryWindow, oam_window, OAM, WINDOW, GtkWindow);

ObjectAttributeMemoryWindow *oam_window_new(DebuggerApp *app);

void oam_window_update(ObjectAttributeMemoryWindow *window, PPU *ppu);

#endif