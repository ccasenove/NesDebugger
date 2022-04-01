#ifndef _PPU_REGISTERS_WIN_H_
#define _PPU_REGISTERS_WIN_H_

#include <gtk/gtk.h>

#include "debugger_app.h"
#include "ppu.h"

#define PPU_REGISTERS_WINDOW_TYPE (ppu_registers_window_get_type())
G_DECLARE_FINAL_TYPE(PPURegistersWindow, ppu_registers_window, PPU, REGISTERS_WINDOW, GtkWindow);

PPURegistersWindow *ppu_registers_window_new(DebuggerApp *app);

void update_ppu_registers_window(PPURegistersWindow *win, PPU *ppu);

#endif