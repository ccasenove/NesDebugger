#include <gtk/gtk.h>

#include "breakpoint_win.h"

struct _BreakpointWindow
{
    GtkWindow parent;
    GtkTreeView *breakpoint_tree_view;
    GtkButton *add_address_bp_button;
    GtkButton *add_memory_bp_button;
    GtkButton *remove_button;
    GtkEntry *address_entry;
};

G_DEFINE_TYPE(BreakpointWindow, breakpoint_window, GTK_TYPE_WINDOW);

static void breakpoint_window_init(BreakpointWindow *window)
{
    gtk_widget_init_template(GTK_WIDGET(window));
}

static void breakpoint_window_class_init(BreakpointWindowClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/org/c4z/debuggerapp/breakpoint_window.xml");

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), BreakpointWindow, breakpoint_tree_view);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), BreakpointWindow, add_address_bp_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), BreakpointWindow, add_memory_bp_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), BreakpointWindow, remove_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), BreakpointWindow, address_entry);
}

static void close_breakpoint_window(GtkWindow *win, DebuggerApp *app)
{
    app->breakpoint_window = NULL;
}

static void close_oam_window(GtkWindow *win, DebuggerApp *app)
{
    app->oam_window = NULL;
}

static void add_address_breakpoint(GtkButton *button, DebuggerApp *app)
{
    BreakpointWindow *breakpoint_window = BREAKPOINT_WINDOW(app->breakpoint_window);

    GtkTreeIter iter;

    gtk_list_store_append(app->breakpoints, &iter);

    gtk_list_store_set(app->breakpoints, &iter, 0, BREAKPOINT_TYPE_ADDRESS, 1, gtk_entry_get_text(breakpoint_window->address_entry), -1);
}

static void add_memory_breakpoint(GtkButton *button, DebuggerApp *app)
{
    BreakpointWindow *breakpoint_window = BREAKPOINT_WINDOW(app->breakpoint_window);

    GtkTreeIter iter;

    gtk_list_store_append(app->breakpoints, &iter);

    gtk_list_store_set(app->breakpoints, &iter, 0, BREAKPOINT_TYPE_MEMORY, 1, gtk_entry_get_text(breakpoint_window->address_entry), -1);
}

static void remove_breakpoint(GtkButton *button, DebuggerApp *app)
{
    BreakpointWindow *breakpoint_window = BREAKPOINT_WINDOW(app->breakpoint_window);

    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkTreeSelection *selection = gtk_tree_view_get_selection(breakpoint_window->breakpoint_tree_view);

    if (gtk_tree_selection_count_selected_rows(selection))
    {
        gtk_tree_selection_get_selected(selection, &model, &iter);
        gtk_list_store_remove(app->breakpoints, &iter);
    }
}

BreakpointWindow *breakpoint_window_new(DebuggerApp *app)
{
    BreakpointWindow *window = g_object_new(BREAKPOINT_WINDOW_TYPE, NULL);

    g_signal_connect(window, "destroy", G_CALLBACK(close_breakpoint_window), app);

    gtk_tree_view_set_model(window->breakpoint_tree_view, GTK_TREE_MODEL(app->breakpoints));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column;
    column = gtk_tree_view_column_new_with_attributes("Type", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(window->breakpoint_tree_view, column);
    column = gtk_tree_view_column_new_with_attributes("Value", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(window->breakpoint_tree_view, column);

    g_signal_connect(window->add_address_bp_button, "clicked", G_CALLBACK(add_address_breakpoint), app);
    g_signal_connect(window->add_memory_bp_button, "clicked", G_CALLBACK(add_memory_breakpoint), app);
    g_signal_connect(window->remove_button, "clicked", G_CALLBACK(remove_breakpoint), app);

    return window;
}
