#include <gtk/gtk.h>

#include "ppu_tables_win.h"

struct _PPUTablesWindow
{
    GtkWindow parent;
    GtkDrawingArea *pattern_table_left_area;
    GtkDrawingArea *pattern_table_right_area;
    GtkGrid *name_table_0_grid;
};

typedef struct _DrawPatternTableCallbackArgs
{
    PPU *ppu;
    unsigned char table_index;
} DrawPatternTableCallbackArgs;

G_DEFINE_TYPE(PPUTablesWindow, ppu_tables_window, GTK_TYPE_WINDOW);

static void ppu_tables_window_init(PPUTablesWindow *window)
{
    gtk_widget_init_template(GTK_WIDGET(window));
}

static void ppu_tables_window_class_init(PPUTablesWindowClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/org/c4z/debuggerapp/ppu_tables_window.xml");

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPUTablesWindow, pattern_table_left_area);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPUTablesWindow, pattern_table_right_area);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPUTablesWindow, name_table_0_grid);
}

static void close_ppu_tables_window(GtkWindow *win, DebuggerApp *app)
{
    app->ppu_tables_window = NULL;
}

static gboolean draw_pattern_table(GtkWidget *widget, cairo_t *cr, DrawPatternTableCallbackArgs *args)
{
    guint width = gtk_widget_get_allocated_width(widget);
    guint height = gtk_widget_get_allocated_height(widget);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    for (int row = 0; row < 16; row++)
    {
        for (int col = 0; col < 16; col++)
        {
            for (int row_number = 0; row_number < 8; row_number++)
            {
                unsigned short lower_row_address = (args->table_index << 12) | (row << 8) | (col << 4) | row_number;
                unsigned short upper_row_address = lower_row_address + 8;

                unsigned char lower_row = ppu_memory_read(args->ppu->ppu_memory, lower_row_address);
                unsigned char upper_row = ppu_memory_read(args->ppu->ppu_memory, upper_row_address);

                unsigned char x = col << 3;
                unsigned char y = (row << 3) | row_number;
                for (int j = 7; j >= 0; j--)
                {
                    unsigned char bit0 = (lower_row >> j) & 0x01;
                    unsigned char bit1 = (upper_row >> j) & 0x01;
                    unsigned char color = bit0 | (bit1 << 1);

                    if (color)
                    {
                        cairo_set_source_rgb(cr, color / (double)3, color / (double)3, color / (double)3);
                        cairo_rectangle(cr, x * 2, y * 2, 2, 2);
                        cairo_fill(cr);
                    }

                    x++;
                }
            }
        }
    }
}

PPUTablesWindow *ppu_tables_window_new(DebuggerApp *app)
{
    PPUTablesWindow *window = g_object_new(PPU_TABLES_WINDOW_TYPE, NULL);

    g_signal_connect(window, "destroy", G_CALLBACK(close_ppu_tables_window), app);

    DrawPatternTableCallbackArgs *args_left = g_new(DrawPatternTableCallbackArgs, 1);
    args_left->ppu = app->nes->ppu;
    args_left->table_index = 0;
    g_signal_connect(window->pattern_table_left_area, "draw", G_CALLBACK(draw_pattern_table), args_left);
    DrawPatternTableCallbackArgs *args_right = g_new(DrawPatternTableCallbackArgs, 1);
    args_right->ppu = app->nes->ppu;
    args_right->table_index = 1;
    g_signal_connect(window->pattern_table_right_area, "draw", G_CALLBACK(draw_pattern_table), args_right);

    GtkWidget *tile_label;
    for (int row = 0; row < 30; row++)
    {
        for (int col = 0; col < 32; col++)
        {
            tile_label = gtk_label_new(".");
            gtk_grid_attach(window->name_table_0_grid, tile_label, col, row, 1, 1);
        }
    }

    return window;
}

void update_ppu_tables_window(PPUTablesWindow *win, PPU *ppu)
{
    if (!win)
    {
        return;
    }

    for (int row = 0; row < 30; row++)
    {
        for (int col = 0; col < 32; col++)
        {
            GtkLabel *label = GTK_LABEL(gtk_grid_get_child_at(win->name_table_0_grid, col, row));

            gchar *text = g_strdup_printf("%02X", ppu_memory_read(ppu->ppu_memory, NAME_TABLE_0 + row * 32 + col));
            gtk_label_set_text(label, text);
            g_free(text);
        }
    }
}