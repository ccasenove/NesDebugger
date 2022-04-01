#ifndef _PPU_TABLES_WIN_H_
#define _PPU_TABLES_WIN_H_

#include <gtk/gtk.h>

#include "debugger_app.h"
#include "ppu.h"

#define PPU_TABLES_WINDOW_TYPE (ppu_tables_window_get_type())
G_DECLARE_FINAL_TYPE(PPUTablesWindow, ppu_tables_window, PPU, TABLES_WINDOW, GtkWindow);

PPUTablesWindow *ppu_tables_window_new(DebuggerApp *app);

void update_ppu_tables_window(PPUTablesWindow *win, PPU *ppu);

#endif