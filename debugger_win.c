#include <gtk/gtk.h>

#include "debugger_app.h"
#include "debugger_win.h"
#include "ppu_registers_win.h"
#include "ppu_tables_win.h"
#include "oam_win.h"
#include "memory_win.h"
#include "disassembler_win.h"
#include "breakpoint_win.h"
#include "system_palette_win.h"
#include "disassembler.h"

struct _DebuggerAppWindow
{
    GtkApplicationWindow parent;
    GtkWidget *open_rom_tool_button;
    GtkEntry *registerA;
    GtkEntry *registerX;
    GtkEntry *registerY;
    GtkCheckButton *n_flag_check_button;
    GtkCheckButton *o_flag_check_button;
    GtkCheckButton *d_flag_check_button;
    GtkCheckButton *i_flag_check_button;
    GtkCheckButton *z_flag_check_button;
    GtkCheckButton *c_flag_check_button;
    GtkEntry *sp;
    GtkEntry *pc;
    GtkLabel *next_instruction_label;
    GtkToolButton *step_button;
    GtkToolButton *run_button;
    GtkToolButton *pause_button;
    GtkMenuItem *ppu_registers_window_menu_item;
    GtkMenuItem *ppu_tables_window_menu_item;
    GtkMenuItem *oam_window_menu_item;
    GtkMenuItem *memory_window_menu_item;
    GtkMenuItem *disassembler_window_menu_item;
    GtkMenuItem *breakpoint_menu_item;
    GtkMenuItem *system_palette_menu_item;
    GtkLabel *start_address_label;
    GtkLabel *nmi_handler_address;
    GtkCheckButton *nmi_simulation_check_button;
};

G_DEFINE_TYPE(DebuggerAppWindow, debugger_app_window, GTK_TYPE_APPLICATION_WINDOW);

static void update_registers(DebuggerAppWindow *win, NES *nes)
{
    gchar *str;

    str = g_strdup_printf("%02X", nes->cpu->registerA);
    gtk_entry_set_text(win->registerA, str);
    g_free(str);

    str = g_strdup_printf("%02X", nes->cpu->registerX);
    gtk_entry_set_text(win->registerX, str);
    g_free(str);

    str = g_strdup_printf("%02X", nes->cpu->registerY);
    gtk_entry_set_text(win->registerY, str);
    g_free(str);

    str = g_strdup_printf("%02X", nes->cpu->sp);
    gtk_entry_set_text(win->sp, str);
    g_free(str);

    str = g_strdup_printf("%04X", nes->cpu->pc);
    gtk_entry_set_text(win->pc, str);
    g_free(str);
}

static void update_status_flags(DebuggerAppWindow *win, unsigned char flags)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->n_flag_check_button), flags & 0x80);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->o_flag_check_button), flags & 0x40);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->d_flag_check_button), flags & 0x08);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->i_flag_check_button), flags & 0x04);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->z_flag_check_button), flags & 0x02);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->c_flag_check_button), flags & 0x01);
}

static void update_next_instruction(DebuggerAppWindow *win, NES *nes)
{
    INSTRUCTION instruction;

    dis_parse_instruction(memory_read_byte(nes->memory, nes->cpu->pc),
                          memory_read_byte(nes->memory, nes->cpu->pc + 1),
                          memory_read_byte(nes->memory, nes->cpu->pc + 2),
                          &instruction);

    char str[32];
    dis_instruction_to_str(&instruction, nes->cpu->pc, str);
    gtk_label_set_text(win->next_instruction_label, str);
}

static void update_memory_windows(DebuggerApp *app)
{
    for (int i = 0; i < NB_MEMORY_WINDOW; i++)
    {
        if (app->memory_windows[i])
        {
            update_memory_window(MEMORY_WINDOW(app->memory_windows[i]), app->nes->memory);
        }
    }
}

void update_debugger_window(DebuggerApp *app)
{
    DebuggerAppWindow *debugger_window = DEBUGGER_APP_WINDOW(app->win);

    update_registers(debugger_window, app->nes);
    update_status_flags(debugger_window, app->nes->cpu->registerP);
    update_next_instruction(debugger_window, app->nes);
    update_ppu_registers_window(PPU_REGISTERS_WINDOW(app->ppu_registers_window), app->nes->ppu);
    update_memory_windows(app);
    update_ppu_tables_window(PPU_TABLES_WINDOW(app->ppu_tables_window), app->nes->ppu);
    oam_window_update(app->oam_window, app->nes->ppu);

    gchar *str;
    str = g_strdup_printf("%04X", memory_read_word(app->nes->memory, 0xfffc));
    gtk_label_set_text(debugger_window->start_address_label, str);
    g_free(str);

    str = g_strdup_printf("%04X", memory_read_word(app->nes->memory, 0xfffa));
    gtk_label_set_text(debugger_window->nmi_handler_address, str);
    g_free(str);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(debugger_window->nmi_simulation_check_button), app->nes->interrupt_NMI);
}

static void open_rom(GtkWidget *widget, DebuggerApp *app)
{
    GtkWidget *dialog;
    gint res;

    dialog = gtk_file_chooser_dialog_new("Open ROM", NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
                                         "_Cancel", GTK_RESPONSE_CANCEL, "_Open", GTK_RESPONSE_ACCEPT, NULL);

    res = gtk_dialog_run(GTK_DIALOG(dialog));

    if (res == GTK_RESPONSE_ACCEPT)
    {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        g_print("%s\n", filename);
        load_rom(app->nes, filename);

        update_debugger_window(app);
    }
    else
    {
        g_print("cancel...\n");
    }

    gtk_widget_destroy(dialog);
}

static void step(GtkToolButton *button, DebuggerApp *app)
{
    execute_instruction(app->nes);

    update_debugger_window(app);
}

static void run_command_cb(GtkToolButton *button, DebuggerApp *app)
{
    debugger_app_run(app);
}

static void pause_command_cb(GtkToolButton *button, DebuggerApp *app)
{
    app->is_running = FALSE;
}

static void open_ppu_registers_window(GtkMenuItem *menu_item, DebuggerApp *app)
{
    if (app->ppu_registers_window)
    {
        gtk_window_present(app->ppu_registers_window);
        return;
    }

    app->ppu_registers_window = GTK_WINDOW(ppu_registers_window_new(app));

    gtk_widget_show_all(GTK_WIDGET(app->ppu_registers_window));
    update_ppu_registers_window(PPU_REGISTERS_WINDOW(app->ppu_registers_window), app->nes->ppu);
}

static void open_ppu_tables_window(GtkMenuItem *menu_item, DebuggerApp *app)
{
    if (app->ppu_tables_window)
    {
        gtk_window_present(app->ppu_tables_window);
        return;
    }

    app->ppu_tables_window = GTK_WINDOW(ppu_tables_window_new(app));

    gtk_widget_show_all(GTK_WIDGET(app->ppu_tables_window));
    update_ppu_tables_window(PPU_TABLES_WINDOW(app->ppu_tables_window), app->nes->ppu);
}

static void open_memory_window(GtkMenuItem *menu_item, DebuggerApp *app)
{
    int i = 0;
    while (i < NB_MEMORY_WINDOW)
    {
        if (!app->memory_windows[i])
        {
            break;
        }
        i++;
    }

    app->memory_windows[i] = GTK_WINDOW(memory_window_new(app));

    gtk_widget_show_all(GTK_WIDGET(app->memory_windows[i]));
    update_memory_window(MEMORY_WINDOW(app->memory_windows[i]), app->nes->memory);
}

static void open_breakpoint_window(GtkMenuItem *menu_item, DebuggerApp *app)
{
    if (app->breakpoint_window)
    {
        gtk_window_present(app->breakpoint_window);
        return;
    }

    app->breakpoint_window = GTK_WINDOW(breakpoint_window_new(app));

    gtk_widget_show_all(GTK_WIDGET(app->breakpoint_window));
}

static void open_system_palette_window(GtkMenuItem *menu_item, DebuggerApp *app)
{
    if (app->system_palette_window)
    {
        gtk_window_present(app->system_palette_window);
        return;
    }

    app->system_palette_window = GTK_WINDOW(system_palette_window_new(app));

    gtk_widget_show_all(GTK_WIDGET(app->system_palette_window));
}

static void open_disassembler_window(GtkMenuItem *menu_item, DebuggerApp *app)
{
    gtk_widget_show_all(GTK_WIDGET(disassembler_window_new(app)));
}

static void open_oam_window(GtkMenuItem *menu_item, DebuggerApp *app)
{
    if (app->oam_window)
    {
        gtk_window_present(app->oam_window);
        return;
    }

    app->oam_window = GTK_WINDOW(oam_window_new(app));

    gtk_widget_show_all(GTK_WIDGET(app->oam_window));
}

static void simulate_nmi(GtkCheckButton *button, DebuggerApp *app)
{
    app->nes->interrupt_NMI = 1;
}

static void debugger_app_window_init(DebuggerAppWindow *window)
{
    gtk_widget_init_template(GTK_WIDGET(window));
}

static void debugger_app_window_class_init(DebuggerAppWindowClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/org/c4z/debuggerapp/window.xml");

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, open_rom_tool_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, registerA);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, registerX);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, registerY);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, sp);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, pc);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, n_flag_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, o_flag_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, d_flag_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, i_flag_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, z_flag_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, c_flag_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, next_instruction_label);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, step_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, run_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, pause_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, ppu_registers_window_menu_item);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, ppu_tables_window_menu_item);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, oam_window_menu_item);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, memory_window_menu_item);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, disassembler_window_menu_item);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, breakpoint_menu_item);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, system_palette_menu_item);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, start_address_label);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, nmi_handler_address);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), DebuggerAppWindow, nmi_simulation_check_button);
}

DebuggerAppWindow *debugger_app_window_new(DebuggerApp *app)
{
    DebuggerAppWindow *window = g_object_new(DEBUGGER_APP_WINDOW_TYPE, "application", app, NULL);

    g_signal_connect(window->open_rom_tool_button, "clicked", G_CALLBACK(open_rom), app);
    g_signal_connect(window->step_button, "clicked", G_CALLBACK(step), app);
    g_signal_connect(window->run_button, "clicked", G_CALLBACK(run_command_cb), app);
    g_signal_connect(window->pause_button, "clicked", G_CALLBACK(pause_command_cb), app);
    g_signal_connect(window->ppu_registers_window_menu_item, "activate", G_CALLBACK(open_ppu_registers_window), app);
    g_signal_connect(window->ppu_tables_window_menu_item, "activate", G_CALLBACK(open_ppu_tables_window), app);
    g_signal_connect(window->oam_window_menu_item, "activate", G_CALLBACK(open_oam_window), app);
    g_signal_connect(window->memory_window_menu_item, "activate", G_CALLBACK(open_memory_window), app);
    g_signal_connect(window->disassembler_window_menu_item, "activate", G_CALLBACK(open_disassembler_window), app);
    g_signal_connect(window->breakpoint_menu_item, "activate", G_CALLBACK(open_breakpoint_window), app);
    g_signal_connect(window->system_palette_menu_item, "activate", G_CALLBACK(open_system_palette_window), app);
    g_signal_connect(window->nmi_simulation_check_button, "clicked", G_CALLBACK(simulate_nmi), app);

    return window;
}
