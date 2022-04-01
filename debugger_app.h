#ifndef _DEBUGGER_APP_H_
#define _DEBUGGER_APP_H_

#include <gtk/gtk.h>

#include "nes.h"

#define NB_MEMORY_WINDOW 16

enum BREAKPOINT_TYPE
{
    BREAKPOINT_TYPE_ADDRESS,
    BREAKPOINT_TYPE_MEMORY
};

struct _DebuggerApp
{
    GtkApplication parent;
    GtkApplicationWindow *win;
    GtkWindow *ppu_registers_window;
    GtkWindow *ppu_tables_window;
    GtkWindow *oam_window;
    GtkWindow *memory_windows[NB_MEMORY_WINDOW];
    GtkWindow *breakpoint_window;
    GtkWindow *system_palette_window;
    NES *nes;
    GtkListStore *breakpoints;
    gboolean is_running;
};

G_DECLARE_FINAL_TYPE(DebuggerApp, debugger_app, DEBUGGER, APP, GtkApplication);

#define DEBUGGER_APP_TYPE (debugger_app_get_type())

DebuggerApp *debugger_app_new();

void debugger_app_run(DebuggerApp *app);

#endif