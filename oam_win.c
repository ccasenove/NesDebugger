#include <gtk/gtk.h>

#include "oam_win.h"

struct _ObjectAttributeMemoryWindow
{
    GtkWindow parent;
    GtkGrid *oam_grid;
};

G_DEFINE_TYPE(ObjectAttributeMemoryWindow, oam_window, GTK_TYPE_WINDOW);

static void oam_window_init(ObjectAttributeMemoryWindow *window)
{
    gtk_widget_init_template(GTK_WIDGET(window));
}

static void oam_window_class_init(ObjectAttributeMemoryWindowClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/org/c4z/debuggerapp/oam_window.xml");

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), ObjectAttributeMemoryWindow, oam_grid);
}

static void oam_window_close_cb(GtkWindow *win, DebuggerApp *app)
{
    app->oam_window = NULL;
}

ObjectAttributeMemoryWindow *oam_window_new(DebuggerApp *app)
{
    ObjectAttributeMemoryWindow *window = g_object_new(OAM_WINDOW_TYPE, NULL);

    g_signal_connect(window, "destroy", G_CALLBACK(oam_window_close_cb), app);

    for (unsigned int i = 0; i < NB_SPRITES; i++)
    {
        GtkWidget *text_view = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
        gtk_grid_attach(window->oam_grid, text_view, i % 4, i / 4, 1, 1);
    }

    oam_window_update(window, app->nes->ppu);

    return window;
}

static void oam_window_set_sprite_attributes(ObjectAttributeMemoryWindow *window, PPU *ppu, unsigned int sprite_index)
{
    GtkTextView *text_view = GTK_TEXT_VIEW(gtk_grid_get_child_at(window->oam_grid, sprite_index % 4, sprite_index / 4));

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    GtkTextIter start;
    GtkTextIter end;

    unsigned int sprite_offset = sprite_index * 4;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_delete(buffer, &start, &end);

    gchar *str;

    gtk_text_buffer_get_end_iter(buffer, &end);
    str = g_strdup_printf("Tile %u\n", ppu->spr_ram[sprite_offset + 1]);
    gtk_text_buffer_insert(buffer, &end, str, -1);
    g_free(str);

    gtk_text_buffer_get_end_iter(buffer, &end);
    str = g_strdup_printf("X: %u\n", ppu->spr_ram[sprite_offset + 3]);
    gtk_text_buffer_insert(buffer, &end, str, -1);
    g_free(str);

    gtk_text_buffer_get_end_iter(buffer, &end);
    str = g_strdup_printf("Y: %u\n", ppu->spr_ram[sprite_offset] + 1);
    gtk_text_buffer_insert(buffer, &end, str, -1);
    g_free(str);

    gtk_text_buffer_get_end_iter(buffer, &end);
    str = g_strdup_printf("----------\n");
    gtk_text_buffer_insert(buffer, &end, str, -1);
    g_free(str);
}

void oam_window_update(ObjectAttributeMemoryWindow *window, PPU *ppu)
{
    if (!window)
    {
        return;
    }

    for (unsigned int i = 0; i < NB_SPRITES; i++)
    {
        oam_window_set_sprite_attributes(window, ppu, i);
    }
}