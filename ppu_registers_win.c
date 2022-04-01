#include <gtk/gtk.h>

#include "ppu_registers_win.h"

struct _PPURegistersWindow
{
    GtkWindow parent;
    GtkCheckButton *ppu_controller_nametable_address_0_check_button;
    GtkCheckButton *ppu_controller_nametable_address_1_check_button;
    GtkCheckButton *ppu_controller_vram_increment_check_button;
    GtkCheckButton *ppu_controller_sprite_table_address_check_button;
    GtkCheckButton *ppu_controller_bg_table_address_check_button;
    GtkCheckButton *ppu_controller_sprite_size_check_button;
    GtkCheckButton *ppu_controller_master_slave_check_button;
    GtkCheckButton *ppu_controller_generate_nmi_check_button;
    GtkLabel *ppu_controller_nametable_address;
    GtkLabel *ppu_controller_vram_increment;
    GtkLabel *ppu_controller_sprite_table_address;
    GtkLabel *ppu_controller_bg_table_address;
    GtkLabel *ppu_controller_sprite_size;
    GtkLabel *ppu_controller_generate_nmi;
    GtkCheckButton *ppu_mask_greyscale_check_button;
    GtkCheckButton *ppu_mask_show_bg_leftmost_pixels_check_button;
    GtkCheckButton *ppu_mask_show_sprites_leftmost_pixels_check_button;
    GtkCheckButton *ppu_mask_show_bg_check_button;
    GtkCheckButton *ppu_mask_show_sprites_check_button;
    GtkCheckButton *ppu_mask_emphasize_red_check_button;
    GtkCheckButton *ppu_mask_emphasize_green_check_button;
    GtkCheckButton *ppu_mask_emphasize_blue_check_button;
    GtkCheckButton *ppu_status_sprite_overflow_check_button;
    GtkCheckButton *ppu_status_sprite_0_hit_check_button;
    GtkCheckButton *ppu_status_vblank_check_button;
};

G_DEFINE_TYPE(PPURegistersWindow, ppu_registers_window, GTK_TYPE_WINDOW);

static void ppu_registers_window_init(PPURegistersWindow *window)
{
    gtk_widget_init_template(GTK_WIDGET(window));
}

static void ppu_registers_window_class_init(PPURegistersWindowClass *class)
{
    gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/org/c4z/debuggerapp/ppu_registers_window.xml");

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_controller_nametable_address);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_controller_vram_increment);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_controller_sprite_table_address);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_controller_bg_table_address);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_controller_sprite_size);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_controller_generate_nmi);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_controller_nametable_address_0_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_controller_nametable_address_1_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_controller_vram_increment_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_controller_sprite_table_address_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_controller_bg_table_address_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_controller_sprite_size_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_controller_master_slave_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_controller_generate_nmi_check_button);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_mask_greyscale_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_mask_show_bg_leftmost_pixels_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_mask_show_sprites_leftmost_pixels_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_mask_show_bg_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_mask_show_sprites_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_mask_emphasize_red_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_mask_emphasize_green_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_mask_emphasize_blue_check_button);

    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_status_sprite_overflow_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_status_sprite_0_hit_check_button);
    gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), PPURegistersWindow, ppu_status_vblank_check_button);
}

static void close_ppu_registers_window(GtkWindow *win, DebuggerApp *app)
{
    app->ppu_registers_window = NULL;
}

static void ppu_status_vblank_clicked(GtkToggleButton *button, DebuggerApp *app)
{
    if (gtk_toggle_button_get_active(button))
    {
        app->nes->ppu->status_register |= 0x80;
    }
    else
    {
        app->nes->ppu->status_register &= ~0x80;
    }
}

PPURegistersWindow *ppu_registers_window_new(DebuggerApp *app)
{
    PPURegistersWindow *window = g_object_new(PPU_REGISTERS_WINDOW_TYPE, NULL);

    g_signal_connect(window, "destroy", G_CALLBACK(close_ppu_registers_window), app);
    g_signal_connect(window->ppu_status_vblank_check_button, "clicked", G_CALLBACK(ppu_status_vblank_clicked), app);

    return window;
}

static void update_ppu_status_register(PPURegistersWindow *win, PPU *ppu)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_status_sprite_overflow_check_button), ppu->status_register & 0x20);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_status_sprite_0_hit_check_button), ppu->status_register & 0x40);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_status_vblank_check_button), ppu->status_register & 0x80);
}

static void update_ppu_mask_register(PPURegistersWindow *win, PPU *ppu)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_mask_greyscale_check_button), ppu->mask_register & 0x01);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_mask_show_bg_leftmost_pixels_check_button), ppu->mask_register & 0x02);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_mask_show_sprites_leftmost_pixels_check_button), ppu->mask_register & 0x04);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_mask_show_bg_check_button), ppu->mask_register & 0x08);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_mask_show_sprites_check_button), ppu->mask_register & 0x10);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_mask_emphasize_red_check_button), ppu->mask_register & 0x20);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_mask_emphasize_green_check_button), ppu->mask_register & 0x40);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_mask_emphasize_blue_check_button), ppu->mask_register & 0x80);
}

static void update_ppu_control_register(PPURegistersWindow *win, PPU *ppu)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_controller_nametable_address_0_check_button), ppu->control_register & 0x01);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_controller_nametable_address_1_check_button), ppu->control_register & 0x02);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_controller_vram_increment_check_button), ppu->control_register & 0x04);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_controller_sprite_table_address_check_button), ppu->control_register & 0x08);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_controller_bg_table_address_check_button), ppu->control_register & 0x10);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_controller_sprite_size_check_button), ppu->control_register & 0x20);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_controller_master_slave_check_button), ppu->control_register & 0x40);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->ppu_controller_generate_nmi_check_button), ppu->control_register & 0x80);

    gchar *str;

    unsigned short nametable_address = NAME_TABLE_0 + (ppu->control_register & 0x03) * 0x400;
    str = g_strdup_printf("$%04X", nametable_address);
    gtk_label_set_text(win->ppu_controller_nametable_address, str);
    g_free(str);

    gtk_label_set_text(win->ppu_controller_vram_increment, ppu->control_register & 0x04 ? "32" : "1");

    gtk_label_set_text(win->ppu_controller_sprite_table_address, ppu->control_register & 0x08 ? "$1000" : "$0000");

    gtk_label_set_text(win->ppu_controller_bg_table_address, ppu->control_register & 0x10 ? "$1000" : "$0000");

    gtk_label_set_text(win->ppu_controller_sprite_size, ppu->control_register & 0x20 ? "8x16" : "8x8");

    gtk_label_set_text(win->ppu_controller_generate_nmi, ppu->control_register & 0x80 ? "on" : "off");
}

void update_ppu_registers_window(PPURegistersWindow *win, PPU *ppu)
{
    if (!win)
    {
        return;
    }

    update_ppu_control_register(win, ppu);
    update_ppu_mask_register(win, ppu);
    update_ppu_status_register(win, ppu);
}