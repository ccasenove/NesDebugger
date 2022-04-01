#ifndef _BREAKPOINT_WIN_H_
#define _BREAKPOINT_WIN_H_

#include <gtk/gtk.h>

#include "debugger_app.h"

#define BREAKPOINT_WINDOW_TYPE (breakpoint_window_get_type())
G_DECLARE_FINAL_TYPE(BreakpointWindow, breakpoint_window, BREAKPOINT, WINDOW, GtkWindow);

BreakpointWindow *breakpoint_window_new(DebuggerApp *app);

#endif