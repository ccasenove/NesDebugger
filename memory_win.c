#include <gtk/gtk.h>
#include <ctype.h>

#include "memory_win.h"

struct _MemoryWindow
{
    GtkWindow parent;
    GtkEntry *memory_start_address_entry;
    GtkTextView *memory_text_view;
    unsigned short start_address;
};

G_DEFINE_TYPE(MemoryWindow, memory_window, GTK_TYPE_WINDOW);

static void memory_window_init(MemoryWindow *window)
{
    gtk_widget_init_template(GTK_WIDGET(window));

    window->start_address = 0;
    gchar *value = g_strdup_printf("%04X", window->start_address);
    gtk_entry_set_text(window->memory_start_address_entry, value);
    g_free(value);
}

static void memory_window_class_init(MemoryWindowClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/org/c4z/debuggerapp/memory_window.xml");

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), MemoryWindow, memory_start_address_entry);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), MemoryWindow, memory_text_view);
}

static void close_memory_window(GtkWindow *window, DebuggerApp *app)
{
    for (int i = 0; i < NB_MEMORY_WINDOW; i++)
    {
        if (app->memory_windows[i] == window)
        {
            app->memory_windows[i] = NULL;
        }
    }
}

void update_memory_window(MemoryWindow *window, MEMORY *memory)
{
    if (!window)
    {
        return;
    }

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(window->memory_text_view);
    GtkTextIter start;
    GtkTextIter end;

    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);

    gtk_text_buffer_delete(buffer, &start, &end);

    unsigned short start_address = window->start_address;

    for (int i = 0; i < 100; i++)
    {
        gchar *str = g_strdup_printf("%04X    %02X %02X %02X %02X %02X %02X %02X %02X    %c%c%c%c%c%c%c%c\n", start_address,
                                     memory_read_byte(memory, start_address),
                                     memory_read_byte(memory, start_address + 1),
                                     memory_read_byte(memory, start_address + 2),
                                     memory_read_byte(memory, start_address + 3),
                                     memory_read_byte(memory, start_address + 4),
                                     memory_read_byte(memory, start_address + 5),
                                     memory_read_byte(memory, start_address + 6),
                                     memory_read_byte(memory, start_address + 7),
                                     isprint(memory_read_byte(memory, start_address)) ? memory_read_byte(memory, start_address) : 0x2e,
                                     isprint(memory_read_byte(memory, start_address + 1)) ? memory_read_byte(memory, start_address + 1) : 0x2e,
                                     isprint(memory_read_byte(memory, start_address + 2)) ? memory_read_byte(memory, start_address + 2) : 0x2e,
                                     isprint(memory_read_byte(memory, start_address + 3)) ? memory_read_byte(memory, start_address + 3) : 0x2e,
                                     isprint(memory_read_byte(memory, start_address + 4)) ? memory_read_byte(memory, start_address + 4) : 0x2e,
                                     isprint(memory_read_byte(memory, start_address + 5)) ? memory_read_byte(memory, start_address + 5) : 0x2e,
                                     isprint(memory_read_byte(memory, start_address + 6)) ? memory_read_byte(memory, start_address + 6) : 0x2e,
                                     isprint(memory_read_byte(memory, start_address + 7)) ? memory_read_byte(memory, start_address + 7) : 0x2e);

        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_insert(buffer, &end, str, -1);
        g_free(str);

        start_address += 8;
    }
}

static void memory_start_address_changed(GtkEntry *entry, DebuggerApp *app)
{
    MemoryWindow *memory_window = MEMORY_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(entry)));

    memory_window->start_address = strtol(gtk_entry_get_text(entry), NULL, 16);
    update_memory_window(memory_window, app->nes->memory);
}

MemoryWindow *memory_window_new(DebuggerApp *app)
{
    MemoryWindow *window = g_object_new(MEMORY_WINDOW_TYPE, NULL);
    g_signal_connect(window, "destroy", G_CALLBACK(close_memory_window), app);
    g_signal_connect(window->memory_start_address_entry, "activate", G_CALLBACK(memory_start_address_changed), app);

    return window;
}
