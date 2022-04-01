#include <gtk/gtk.h>
#include <ctype.h>

#include "system_palette_win.h"
#include "ppu.h"

struct _SystemPaletteWindow
{
    GtkWindow parent;
    GtkGrid *grid;
};

G_DEFINE_TYPE(SystemPaletteWindow, system_palette_window, GTK_TYPE_WINDOW);

static void system_palette_window_init(SystemPaletteWindow *window)
{
    gtk_widget_init_template(GTK_WIDGET(window));
}

static void system_palette_window_class_init(SystemPaletteWindowClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/org/c4z/debuggerapp/system_palette_window.xml");

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), SystemPaletteWindow, grid);
}

static void close_system_palette_window(GtkWindow *win, DebuggerApp *app)
{
    app->system_palette_window = NULL;
}

static gboolean draw_bg_color(GtkWidget *widget, cairo_t *cr, COLOR *color)
{
    guint width = gtk_widget_get_allocated_width(widget);
    guint height = gtk_widget_get_allocated_height(widget);
    cairo_set_source_rgb(cr, color->red / (double)256, color->green / (double)256, color->blue / (double)256);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
}

SystemPaletteWindow *system_palette_window_new(DebuggerApp *app)
{
    SystemPaletteWindow *window = g_object_new(SYSTEM_PALETTE_WINDOW_TYPE, NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(close_system_palette_window), app);

    GtkDrawingArea *area;
    gint col = 1;
    gint row = 1;
    for (int i = 0; i < sizeof(SYSTEM_PALETTE) / sizeof(COLOR); i++)
    {
        area = GTK_DRAWING_AREA(gtk_drawing_area_new());
        g_signal_connect(area, "draw", G_CALLBACK(draw_bg_color), &SYSTEM_PALETTE[i]);

        gtk_grid_attach(window->grid, GTK_WIDGET(area), col, row, 1, 1);

        row++;
        if (row > 16)
        {
            row = 1;
            col++;
        }
    }
    return window;
}
