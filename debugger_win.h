#ifndef _DEBUGGER_WIN_H_
#define _DEBUGGER_WIN_H_

#include <gtk/gtk.h>

#include "debugger_app.h"

#define DEBUGGER_APP_WINDOW_TYPE (debugger_app_window_get_type())
G_DECLARE_FINAL_TYPE(DebuggerAppWindow, debugger_app_window, DEBUGGER, APP_WINDOW, GtkApplicationWindow);

DebuggerAppWindow *debugger_app_window_new(DebuggerApp *app);

void update_debugger_window(DebuggerApp *app);

#endif