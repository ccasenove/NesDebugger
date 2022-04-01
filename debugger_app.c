#include <gtk/gtk.h>

#include "debugger_app.h"
#include "debugger_win.h"

G_DEFINE_TYPE(DebuggerApp, debugger_app, GTK_TYPE_APPLICATION);

static void debugger_app_activate(GApplication *app)
{
    DebuggerAppWindow *win = debugger_app_window_new(DEBUGGER_APP(app));

    DebuggerApp *debugger_app = DEBUGGER_APP(app);
    debugger_app->win = GTK_APPLICATION_WINDOW(win);

    gtk_window_present(GTK_WINDOW(win));
}

static void debugger_app_class_init(DebuggerAppClass *class)
{
    G_APPLICATION_CLASS(class)->activate = debugger_app_activate;
}

static void debugger_app_init(DebuggerApp *app)
{
    app->nes = create_nes();
    app->breakpoints = gtk_list_store_new(2, G_TYPE_UINT, G_TYPE_STRING);
}

DebuggerApp *debugger_app_new()
{
    return g_object_new(DEBUGGER_APP_TYPE, "application-id", "org.c4z.nesdbg",
                        "flags", G_APPLICATION_FLAGS_NONE, NULL);
}

void debugger_app_load_rom(DebuggerApp *app, const char *filename)
{
    load_rom(app->nes, filename);
}

static gboolean breakpoint_matches(GtkListStore *breakpoints, enum BREAKPOINT_TYPE breakpoint_type, unsigned short address)
{
    GtkTreeIter iter;
    gchar *value;
    gint type;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(breakpoints), &iter);

    while (valid)
    {
        gtk_tree_model_get(GTK_TREE_MODEL(breakpoints), &iter, 0, &type, 1, &value, -1);

        if (type == breakpoint_type && address == strtol(value, NULL, 16))
        {
            g_free(value);
            return TRUE;
        }

        g_free(value);

        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(breakpoints), &iter);
    }

    return FALSE;
}

static gboolean run_function(DebuggerApp *app)
{
    if (breakpoint_matches(app->breakpoints, BREAKPOINT_TYPE_ADDRESS, app->nes->cpu->pc))
        return G_SOURCE_REMOVE;

    execute_instruction(app->nes);
    update_debugger_window(app);

    if (breakpoint_matches(app->breakpoints, BREAKPOINT_TYPE_MEMORY, app->nes->memory->last_read_address) || breakpoint_matches(app->breakpoints, BREAKPOINT_TYPE_MEMORY, app->nes->memory->last_write_address))
        return G_SOURCE_REMOVE;

    return app->is_running ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;
}

void debugger_app_run(DebuggerApp *app)
{
    app->is_running = TRUE;
    g_idle_add(G_SOURCE_FUNC(run_function), app);
}