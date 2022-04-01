#include <gtk/gtk.h>
#include <ctype.h>

#include "disassembler_win.h"
#include "disassembler.h"

struct _DisassemblerWindow
{
    GtkWindow parent;
    GtkEntry *disassembler_start_address_entry;
    GtkTextView *disassembler_text_view;
    unsigned short start_address;
};

G_DEFINE_TYPE(DisassemblerWindow, disassembler_window, GTK_TYPE_WINDOW);

static void disassembler_window_init(DisassemblerWindow *window)
{
    gtk_widget_init_template(GTK_WIDGET(window));
}

static void disassembler_window_class_init(DisassemblerWindowClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/org/c4z/debuggerapp/disassembler_window.xml");

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DisassemblerWindow, disassembler_start_address_entry);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DisassemblerWindow, disassembler_text_view);
}

static void update_disassembler_window(DisassemblerWindow *window, NES *nes)
{
    if (!window)
    {
        return;
    }

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(window->disassembler_text_view);
    GtkTextIter start;
    GtkTextIter end;

    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);

    gtk_text_buffer_delete(buffer, &start, &end);

    unsigned short address = window->start_address;
    INSTRUCTION instruction;
    char str[32];
    for (int i = 0; i < 100; i++)
    {
        dis_parse_instruction(memory_read_byte(nes->memory, address),
                              memory_read_byte(nes->memory, address + 1),
                              memory_read_byte(nes->memory, address + 2), &instruction);

        dis_instruction_to_str(&instruction, address, str);

        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_insert(buffer, &end, str, -1);
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_insert(buffer, &end, "\n", -1);

        address += instruction.length;
    }
}

static void disassembler_start_address_changed(GtkEntry *entry, DebuggerApp *app)
{
    DisassemblerWindow *disassembler_window = DISASSEMBLER_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(entry)));

    disassembler_window->start_address = strtol(gtk_entry_get_text(entry), NULL, 16);
    update_disassembler_window(disassembler_window, app->nes);
}

DisassemblerWindow *disassembler_window_new(DebuggerApp *app)
{
    DisassemblerWindow *window = g_object_new(DISASSEMBLER_WINDOW_TYPE, NULL);

    window->start_address = app->nes->cpu->pc;
    gchar *value = g_strdup_printf("%04X", window->start_address);
    gtk_entry_set_text(window->disassembler_start_address_entry, value);
    g_free(value);

    update_disassembler_window(window, app->nes);

    g_signal_connect(window->disassembler_start_address_entry, "activate", G_CALLBACK(disassembler_start_address_changed), app);

    return window;
}
