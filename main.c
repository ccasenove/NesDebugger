#include <gtk/gtk.h>

#include "debugger_app.h"

int main(int argc, char *argv[])
{
    DebuggerApp *app;
    app = debugger_app_new();

    return g_application_run(G_APPLICATION(app), argc, argv);
}