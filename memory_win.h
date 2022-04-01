#ifndef _MEMORY_WIN_H_
#define _MEMORY_WIN_H_

#include <gtk/gtk.h>

#include "debugger_app.h"

#define MEMORY_WINDOW_TYPE (memory_window_get_type())
G_DECLARE_FINAL_TYPE(MemoryWindow, memory_window, MEMORY, WINDOW, GtkWindow);

MemoryWindow *memory_window_new(DebuggerApp *app);

void update_memory_window(MemoryWindow *window, MEMORY *memory);

#endif