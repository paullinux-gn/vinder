/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <config.h>

#include "vinder-interface.h"

#include <glib/gi18n.h>

#include "nautilus-global-preferences.h"

static void
update_appearance (GSettings           *settings,
                   const char          *key,
                   NautilusApplication *self)
{
    g_autofree char *appearance = g_settings_get_string (settings, key);
    GAction *action = g_action_map_lookup_action (G_ACTION_MAP (self), key);
    gboolean mixed = g_str_equal (appearance, "dark-and-light");

    g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_string (appearance));
    adw_style_manager_set_color_scheme (adw_style_manager_get_default (),
                                       g_str_equal (appearance, "light")
                                       ? ADW_COLOR_SCHEME_FORCE_LIGHT
                                       : ADW_COLOR_SCHEME_FORCE_DARK);

    for (GList *l = nautilus_application_get_windows (self); l != NULL; l = l->next)
    {
        GtkWidget *window = GTK_WIDGET (l->data);

        if (mixed)
        {
            gtk_widget_add_css_class (window, "vinder-dark-and-light");
        }
        else
        {
            gtk_widget_remove_css_class (window, "vinder-dark-and-light");
        }
    }
}

static void
change_appearance (GSimpleAction *action,
                   GVariant      *value,
                   gpointer       user_data)
{
    g_settings_set_value (nautilus_preferences, g_action_get_name (G_ACTION (action)), value);
}

static void
update_blur (GSettings           *settings,
             const char          *key,
             NautilusApplication *self)
{
    gboolean enabled = g_settings_get_boolean (settings, "vinder-blur");
    GActionMap *actions = G_ACTION_MAP (self);
    GAction *toggle = g_action_map_lookup_action (actions, "vinder-blur");
    g_autofree char *css = NULL;

    g_simple_action_set_state (G_SIMPLE_ACTION (toggle), g_variant_new_boolean (enabled));

    if (enabled)
    {
        double opacity = g_settings_get_double (settings, "vinder-blur-opacity");
        double strength = g_settings_get_double (settings, "vinder-blur-strength");
        g_autofree char *menu_css = NULL;

        if (g_settings_get_boolean (settings, "vinder-menu-blur"))
        {
            menu_css = g_strdup_printf (
                "  .vinder-window popover > contents,"
                "  .vinder-window .floating-bar,"
                "  .vinder-window popover > arrow {"
                "    backdrop-filter: blur(%dpx);"
                "    background-color: color-mix(in srgb, var(--popover-bg-color) %d%%, transparent);"
                "  }", (int) strength, (int) opacity);
        }

        css = g_strdup_printf (
            "@media (prefers-contrast: no-preference) {"
            "  window.view.vinder-window { background-color: transparent; }"
            "  .vinder-content { background-color: var(--view-bg-color); }"
            "  .vinder-content.scroll-animation {"
            "    --vinder-header-tint: color-mix(in srgb, var(--headerbar-bg-color) %d%%, transparent);"
            "  }"
            "  .vinder-content.scroll-animation > .top-bar,"
            "  .vinder-content.scroll-animation > .top-bar:backdrop,"
            "  .vinder-content.scroll-animation .nautilus-list-view columnview > header {"
            "    backdrop-filter: blur(%dpx);"
            "    background: var(--vinder-header-tint);"
            "    box-shadow: none;"
            "  }"
            "  .vinder-content.scroll-animation > .top-bar headerbar {"
            "    background: transparent;"
            "  }"
            "  .vinder-content.scroll-animation .nautilus-grid-view gridview { padding-top: 66px; }"
            "  .vinder-content.scroll-animation .nautilus-list-view columnview > listview {"
            "    margin-top: -112px;"
            "    padding-top: calc(112px + 16px);"
            "  }"
            "  .vinder-content.scroll-animation .nautilus-grid-view scrolledwindow > undershoot.top,"
            "  .vinder-content.scroll-animation .nautilus-list-view scrolledwindow > undershoot.top {"
            "    min-height: 84px;"
            "    background: linear-gradient(to bottom,"
            "      transparent 40px,"
            "      var(--vinder-header-tint) 48px,"
            "      color-mix(in srgb, var(--vinder-header-tint) 55%%, transparent) 60px,"
            "      transparent);"
            "  }"
            "  .vinder-split-view > .sidebar-pane,"
            "  .vinder-content-surround,"
            "  .vinder-split-view > .overlay-pane {"
            "    backdrop-filter: blur(%dpx);"
            "    background-color: color-mix(in srgb, var(--sidebar-bg-color) %d%%, transparent);"
            "  }"
            "  .vinder-split-view > .sidebar-pane:backdrop,"
            "  .vinder-content-surround:backdrop {"
            "    background-color: color-mix(in srgb, var(--sidebar-backdrop-color) %d%%, transparent);"
            "  }"
            "  dialog-host > dialog sheet,"
            "  window.background:not(.vinder-window) {"
            "    --vinder-popup-bg: var(--window-bg-color);"
            "    backdrop-filter: blur(%dpx);"
            "    background-color: color-mix(in srgb, var(--vinder-popup-bg) %d%%, transparent);"
            "  }"
            "  dialog-host > dialog.alert sheet,"
            "  window.background.alert:not(.vinder-window),"
            "  window.background.messagedialog:not(.vinder-window) {"
            "    --vinder-popup-bg: var(--dialog-bg-color);"
            "  }"
            "  %s"
            "}", MIN ((int) opacity, 62), MIN ((int) strength, 18), (int) strength, (int) opacity, (int) opacity,
                 (int) strength, (int) opacity, menu_css != NULL ? menu_css : "");
    }

    GtkCssProvider *provider = g_object_get_data (G_OBJECT (self), "blur-provider");

    gtk_css_provider_load_from_string (provider, css != NULL ? css : "");
}

static void
change_blur (GSimpleAction *action,
             GVariant      *value,
             gpointer       user_data)
{
    GtkWindow *window = gtk_application_get_active_window (GTK_APPLICATION (user_data));

    if (!g_settings_set_value (nautilus_preferences, "vinder-blur", value) ||
        !g_variant_get_boolean (value) || window == NULL ||
        g_settings_get_boolean (nautilus_preferences, "vinder-blur-reminder-seen"))
    {
        return;
    }

    AdwDialog *dialog = adw_alert_dialog_new (_("Sidebar Blur"),
                                              _("Desktop blur on GNOME requires GNOME 51 or later."));

    adw_alert_dialog_add_response (ADW_ALERT_DIALOG (dialog), "ok", _("_OK"));
    adw_alert_dialog_set_default_response (ADW_ALERT_DIALOG (dialog), "ok");
    adw_alert_dialog_set_close_response (ADW_ALERT_DIALOG (dialog), "ok");
    adw_dialog_present (dialog, GTK_WIDGET (window));
    g_settings_set_boolean (nautilus_preferences, "vinder-blur-reminder-seen", TRUE);
}

static void
update_icon_shadow_strength (GSettings           *settings,
                             const char          *key,
                             NautilusApplication *self)
{
    int strength = (int) g_settings_get_double (settings, "vinder-icon-shadow-strength");
    g_autofree char *css = g_strdup_printf (
        ".vinder-content.pop-icons { --vinder-icon-shadow-strength: %d%%;"
        " --vinder-icon-shadow-lift: %d; }", strength, strength);

    GtkCssProvider *provider = g_object_get_data (G_OBJECT (self), "icon-shadow-provider");

    gtk_css_provider_load_from_string (provider, css);
}

static GtkWidget *
add_interface_slider (AdwPreferencesGroup *group,
                      const char          *title,
                      const char          *key,
                      int                  maximum)
{
    AdwActionRow *row = ADW_ACTION_ROW (adw_action_row_new ());
    GtkWidget *scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0, maximum, 1);

    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (row), title);
    gtk_scale_set_draw_value (GTK_SCALE (scale), TRUE);
    gtk_scale_set_digits (GTK_SCALE (scale), 0);
    gtk_widget_set_size_request (scale, 150, -1);
    gtk_widget_set_valign (scale, GTK_ALIGN_CENTER);
    gtk_accessible_update_property (GTK_ACCESSIBLE (scale),
                                    GTK_ACCESSIBLE_PROPERTY_LABEL, title, -1);
    g_settings_bind (nautilus_preferences, key,
                     gtk_range_get_adjustment (GTK_RANGE (scale)), "value",
                     G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_NO_SENSITIVITY);
    adw_action_row_add_suffix (row, scale);
    adw_preferences_group_add (group, GTK_WIDGET (row));

    return GTK_WIDGET (row);
}

static void
action_interface (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
    GtkWindow *window = gtk_application_get_active_window (GTK_APPLICATION (user_data));
    AdwDialog *dialog = adw_preferences_dialog_new ();
    AdwPreferencesPage *page = ADW_PREFERENCES_PAGE (adw_preferences_page_new ());
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());

    AdwPreferencesGroup *themes = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());
    const char *theme_labels[] = { _("Light"), _("Dark"), _("Dark and Light") };
    const char *theme_values[] = { "light", "dark", "dark-and-light" };
    GtkCheckButton *first = NULL;
    AdwSwitchRow *blur = ADW_SWITCH_ROW (adw_switch_row_new ());
    AdwSwitchRow *menu_blur = ADW_SWITCH_ROW (adw_switch_row_new ());
    AdwSwitchRow *scroll_animation = ADW_SWITCH_ROW (adw_switch_row_new ());
    AdwSwitchRow *pop_icons = ADW_SWITCH_ROW (adw_switch_row_new ());

    adw_dialog_set_title (dialog, _("Interface"));
    adw_dialog_set_content_width (dialog, 360);
    adw_preferences_group_set_title (themes, _("Appearance"));
    for (guint i = 0; i < G_N_ELEMENTS (theme_labels); i++)
    {
        GtkCheckButton *choice = GTK_CHECK_BUTTON (gtk_check_button_new_with_label (theme_labels[i]));

        if (first == NULL)
        {
            first = choice;
        }
        else
        {
            gtk_check_button_set_group (choice, first);
        }
        gtk_actionable_set_action_target (GTK_ACTIONABLE (choice), "s", theme_values[i]);
        gtk_actionable_set_action_name (GTK_ACTIONABLE (choice), "app.vinder-appearance");
        adw_preferences_group_add (themes, GTK_WIDGET (choice));
    }
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (pop_icons), _("Pop the Icons"));
    g_settings_bind (nautilus_preferences, "vinder-pop-icons", pop_icons, "active",
                     G_SETTINGS_BIND_DEFAULT);
    adw_preferences_group_add (themes, GTK_WIDGET (pop_icons));
    GtkWidget *shadow_strength = add_interface_slider (themes, _("Shadow Strength (%)"),
                                                       "vinder-icon-shadow-strength", 100);
    g_settings_bind (nautilus_preferences, "vinder-pop-icons", shadow_strength, "visible",
                     G_SETTINGS_BIND_GET);
    AdwSwitchRow *zebra_rows = ADW_SWITCH_ROW (adw_switch_row_new ());
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (zebra_rows), _("Alternating List Row Colors"));
    g_settings_bind (nautilus_preferences, "vinder-zebra-rows", zebra_rows, "active",
                     G_SETTINGS_BIND_DEFAULT);
    adw_preferences_group_add (themes, GTK_WIDGET (zebra_rows));
    adw_preferences_page_add (page, themes);

    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (blur), _("Blur"));
    gtk_actionable_set_action_name (GTK_ACTIONABLE (blur), "app.vinder-blur");
    adw_preferences_group_add (group, GTK_WIDGET (blur));
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (menu_blur), _("Blur Menus"));
    g_settings_bind (nautilus_preferences, "vinder-menu-blur", menu_blur, "active",
                     G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_NO_SENSITIVITY);
    g_settings_bind (nautilus_preferences, "vinder-blur", menu_blur, "sensitive", G_SETTINGS_BIND_GET);
    adw_preferences_group_add (group, GTK_WIDGET (menu_blur));
    adw_preferences_row_set_title (ADW_PREFERENCES_ROW (scroll_animation), _("Scroll Animation"));
    g_settings_bind (nautilus_preferences, "vinder-scroll-animation", scroll_animation, "active",
                     G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_NO_SENSITIVITY);
    g_settings_bind (nautilus_preferences, "vinder-blur", scroll_animation, "sensitive", G_SETTINGS_BIND_GET);
    adw_preferences_group_add (group, GTK_WIDGET (scroll_animation));
    adw_preferences_page_add (page, group);

    AdwPreferencesGroup *sliders = ADW_PREFERENCES_GROUP (adw_preferences_group_new ());

    add_interface_slider (sliders, _("Opacity (%)"), "vinder-blur-opacity", 100);
    add_interface_slider (sliders, _("Blur Strength"), "vinder-blur-strength", 40);
    g_settings_bind (nautilus_preferences, "vinder-blur", sliders, "sensitive", G_SETTINGS_BIND_GET);
    adw_preferences_page_add (page, sliders);
    adw_preferences_dialog_add (ADW_PREFERENCES_DIALOG (dialog), page);
    adw_dialog_present (dialog, GTK_WIDGET (window));
}

static const GActionEntry interface_entries[] =
{
    { .name = "vinder-appearance", .parameter_type = "s", .state = "\"light\"", .change_state = change_appearance },
    { .name = "vinder-blur", .state = "false", .change_state = change_blur },
    { .name = "vinder-interface", .activate = action_interface },
};

void
vinder_interface_setup (NautilusApplication *self)
{
    g_action_map_add_action_entries (G_ACTION_MAP (self),
                                     interface_entries, G_N_ELEMENTS (interface_entries),
                                     self);

    g_signal_connect_object (nautilus_preferences, "changed::vinder-appearance",
                             G_CALLBACK (update_appearance), self, 0);
    update_appearance (nautilus_preferences, "vinder-appearance", self);

    GtkCssProvider *blur_provider = gtk_css_provider_new ();
    g_object_set_data_full (G_OBJECT (self), "blur-provider", blur_provider, g_object_unref);
    gtk_style_context_add_provider_for_display (gdk_display_get_default (),
                                                GTK_STYLE_PROVIDER (blur_provider),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    const char *blur_signals[] = {
        "changed::vinder-blur",
        "changed::vinder-menu-blur",
        "changed::vinder-blur-opacity",
        "changed::vinder-blur-strength",
    };
    for (guint i = 0; i < G_N_ELEMENTS (blur_signals); i++)
    {
        g_signal_connect_object (nautilus_preferences, blur_signals[i],
                                 G_CALLBACK (update_blur), self, 0);
    }
    update_blur (nautilus_preferences, NULL, self);

    GtkCssProvider *icon_shadow_provider = gtk_css_provider_new ();
    g_object_set_data_full (G_OBJECT (self), "icon-shadow-provider", icon_shadow_provider, g_object_unref);
    gtk_style_context_add_provider_for_display (gdk_display_get_default (),
                                                GTK_STYLE_PROVIDER (icon_shadow_provider),
                                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_signal_connect_object (nautilus_preferences, "changed::vinder-icon-shadow-strength",
                             G_CALLBACK (update_icon_shadow_strength), self, 0);
    update_icon_shadow_strength (nautilus_preferences, NULL, self);
}

void
vinder_interface_apply_window_appearance (GtkWindow *window)
{
    g_autofree char *appearance = g_settings_get_string (nautilus_preferences, "vinder-appearance");

    if (g_str_equal (appearance, "dark-and-light"))
    {
        gtk_widget_add_css_class (GTK_WIDGET (window), "vinder-dark-and-light");
    }
}
