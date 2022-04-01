#ifndef _DISASSEMBLER_WIN_H_
#define _DISASSEMBLER_WIN_H_

#include <gtk/gtk.h>

#include "debugger_app.h"

#define DISASSEMBLER_WINDOW_TYPE (disassembler_window_get_type())
G_DECLARE_FINAL_TYPE(DisassemblerWindow, disassembler_window, DISASSEMBLER, WINDOW, GtkWindow);

DisassemblerWindow *disassembler_window_new(DebuggerApp *app);

#endif