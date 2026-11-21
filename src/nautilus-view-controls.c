/*
 * SPDX-FileCopyrightText: 2022 The GNOME project contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nautilus-view-controls.h"
#include "nautilus-metadata.h"

#include "nautilus-file.h"
#include "nautilus-files-view.h"
#include "nautilus-view-info.h"
#include "nautilus-global-preferences.h"
#include "nautilus-file-utilities.h"
#include "nautilus-scheme.h"
#include "nautilus-ui-utilities.h"
#include "nautilus-window-slot.h"

/* Assume sort section is second item in view_menu. See nautilus-view-controls.blp */
#define SORT_ITEM_POSITION 1

struct _NautilusViewControls
{
    AdwBin parent_instance;

    GtkWidget *view_split_button;
    AdwToggleGroup *view_toggle_group;
    gboolean updating_buttons;
    GMenuModel *view_menu;
    GMenuModel *sort_section;

    NautilusWindowSlot *window_slot;
};

G_DEFINE_FINAL_TYPE (NautilusViewControls, nautilus_view_controls, ADW_TYPE_BIN);


enum
{
    PROP_0,
    PROP_WINDOW_SLOT,
    N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
update_view_buttons (NautilusViewControls *self)
{
    NautilusFilesView *view = self->window_slot != NULL
                             ? nautilus_window_slot_get_current_view (self->window_slot) : NULL;
    guint view_id = view != NULL ? nautilus_files_view_get_view_id (view) : NAUTILUS_VIEW_INVALID_ID;
    gboolean tree = g_settings_get_boolean (nautilus_list_view_preferences,
                                            NAUTILUS_PREFERENCES_LIST_VIEW_USE_TREE);
    GFile *location = self->window_slot != NULL
                      ? nautilus_window_slot_get_location (self->window_slot) : NULL;
    if (location != NULL)
    {
        g_autoptr (NautilusFile) folder = nautilus_file_get (location);
        const char *mode = vinder_folder_get_view_mode (folder);
        if (mode != NULL)
        {
            tree = g_str_equal (mode, "tree");
        }
    }
    gboolean available = view_id == NAUTILUS_VIEW_GRID_ID || view_id == NAUTILUS_VIEW_LIST_ID;

    self->updating_buttons = TRUE;
    gtk_widget_set_sensitive (GTK_WIDGET (self->view_toggle_group), available);
    adw_toggle_group_set_active_name (self->view_toggle_group,
                                      view_id == NAUTILUS_VIEW_GRID_ID ? "grid" :
                                      tree ? "tree" : "list");
    self->updating_buttons = FALSE;
}

static void
on_tree_preference_changed (GSettings            *settings,
                            const char           *key,
                            NautilusViewControls *self)
{
    update_view_buttons (self);
}

static void
on_view_button_toggled (AdwToggleGroup       *group,
                        GParamSpec           *pspec,
                        NautilusViewControls *self)
{
    if (self->updating_buttons || self->window_slot == NULL)
    {
        return;
    }

    const char *mode = adw_toggle_group_get_active_name (group);
    if (mode == NULL)
    {
        return;
    }
    gboolean grid = g_str_equal (mode, "grid");
    GFile *location = nautilus_window_slot_get_location (self->window_slot);
    if (location == NULL)
    {
        return;
    }

    g_autoptr (NautilusFile) folder = nautilus_file_get (location);
    const char *previous_mode = vinder_folder_get_view_mode (folder);
    gboolean changed = g_strcmp0 (mode, previous_mode) != 0;
    NautilusFilesView *view = nautilus_window_slot_get_current_view (self->window_slot);
    gboolean reload_list = !grid && changed && view != NULL &&
                           nautilus_files_view_get_view_id (view) == NAUTILUS_VIEW_LIST_ID;

    vinder_folder_set_view_mode (folder, mode);

    gtk_widget_activate_action (GTK_WIDGET (self), "slot.files-view-mode", "u",
                                 grid ? NAUTILUS_VIEW_GRID_ID : NAUTILUS_VIEW_LIST_ID);
    if (reload_list)
    {
        gtk_widget_activate_action (GTK_WIDGET (self), "slot.reload", NULL);
    }
    update_view_buttons (self);
}

static void
update_sort_section (NautilusViewControls *self,
                     NautilusFile         *file)
{
    const char *action;

    /* When not in the special location, set an inexistant action to hide the
     * menu item. This works under the assumptiont that the menu item has its
     * "hidden-when" attribute set to "action-disabled", and that an inexistant
     * action is treated as a disabled action. */
    action = nautilus_file_is_in_trash (file) ? "view.sort" : "doesnt-exist";
    nautilus_menu_item_change_attribute (self->sort_section,
                                         "last_trashed",
                                         G_MENU_ATTRIBUTE_ACTION, action);

    action = nautilus_file_is_in_recent (file) ? "view.sort" : "doesnt-exist";
    nautilus_menu_item_change_attribute (self->sort_section,
                                         "recency",
                                         G_MENU_ATTRIBUTE_ACTION, action);

    action = nautilus_file_is_in_search (file) ? "view.sort" : "doesnt-exist";
    nautilus_menu_item_change_attribute (self->sort_section,
                                         "relevance",
                                         G_MENU_ATTRIBUTE_ACTION, action);
}

static void
adjust_to_location (NautilusViewControls *self,
                    GParamSpec           *param,
                    NautilusWindowSlot   *slot)
{
    GFile *location = nautilus_window_slot_get_location (slot);
    gboolean has_menu = location != NULL &&
                        !nautilus_is_root_for_scheme (location, SCHEME_NETWORK_VIEW);

    gtk_widget_set_sensitive (self->view_split_button, has_menu);
    update_view_buttons (self);

    if (!has_menu)
    {
        return;
    }

    g_autoptr (NautilusFile) file = nautilus_file_get (location);

    update_sort_section (self, file);
}

static void
on_tooltip_changed (NautilusViewControls *self,
                    GParamSpec           *param,
                    NautilusWindowSlot   *slot)
{
    update_view_buttons (self);
}


static void
disconnect_toolbar_menu_sections_change_handler (NautilusViewControls *self)
{
    if (self->window_slot == NULL)
    {
        return;
    }

    g_signal_handlers_disconnect_by_func (self->window_slot,
                                          G_CALLBACK (adjust_to_location),
                                          self);
    g_signal_handlers_disconnect_by_func (self->window_slot,
                                          G_CALLBACK (on_tooltip_changed),
                                          self);
}


static void
nautilus_view_controls_set_window_slot (NautilusViewControls *self,
                                        NautilusWindowSlot   *window_slot)
{
    g_return_if_fail (NAUTILUS_IS_VIEW_CONTROLS (self));
    g_return_if_fail (window_slot == NULL || NAUTILUS_IS_WINDOW_SLOT (window_slot));

    if (self->window_slot == window_slot)
    {
        return;
    }

    disconnect_toolbar_menu_sections_change_handler (self);

    self->window_slot = window_slot;

    if (self->window_slot != NULL)
    {
        adjust_to_location (self, NULL, self->window_slot);
        g_signal_connect_swapped (self->window_slot, "notify::location",
                                  G_CALLBACK (adjust_to_location), self);
        g_signal_connect_swapped (self->window_slot, "notify::tooltip",
                                  G_CALLBACK (on_tooltip_changed), self);
    }

    update_view_buttons (self);
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_WINDOW_SLOT]);
}

static void
nautilus_view_controls_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
    NautilusViewControls *self = NAUTILUS_VIEW_CONTROLS (object);

    switch (prop_id)
    {
        case PROP_WINDOW_SLOT:
        {
            g_value_set_object (value, G_OBJECT (self->window_slot));
            break;
        }

        default:
        {
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
    }
}

static void
nautilus_view_controls_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
    NautilusViewControls *self = NAUTILUS_VIEW_CONTROLS (object);

    switch (prop_id)
    {
        case PROP_WINDOW_SLOT:
        {
            nautilus_view_controls_set_window_slot (self, NAUTILUS_WINDOW_SLOT (g_value_get_object (value)));
            break;
        }

        default:
        {
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
    }
}

static void
nautilus_view_controls_dispose (GObject *obj)
{
    NautilusViewControls *self = NAUTILUS_VIEW_CONTROLS (obj);

    disconnect_toolbar_menu_sections_change_handler (self);
    adw_bin_set_child (ADW_BIN (self), NULL);
    gtk_widget_dispose_template (GTK_WIDGET (self), NAUTILUS_TYPE_VIEW_CONTROLS);

    G_OBJECT_CLASS (nautilus_view_controls_parent_class)->dispose (obj);
}

static void
nautilus_view_controls_finalize (GObject *obj)
{
    NautilusViewControls *self = NAUTILUS_VIEW_CONTROLS (obj);

    g_clear_object (&self->sort_section);
    g_clear_object (&self->window_slot);

    G_OBJECT_CLASS (nautilus_view_controls_parent_class)->finalize (obj);
}

static void
nautilus_view_controls_class_init (NautilusViewControlsClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->dispose = nautilus_view_controls_dispose;
    object_class->finalize = nautilus_view_controls_finalize;
    object_class->get_property = nautilus_view_controls_get_property;
    object_class->set_property = nautilus_view_controls_set_property;

    properties[PROP_WINDOW_SLOT] = g_param_spec_object ("window-slot",
                                                        NULL, NULL,
                                                        NAUTILUS_TYPE_WINDOW_SLOT,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/nautilus/ui/nautilus-view-controls.ui");
    gtk_widget_class_bind_template_child (widget_class, NautilusViewControls, view_menu);
    gtk_widget_class_bind_template_child (widget_class, NautilusViewControls, view_split_button);
    gtk_widget_class_bind_template_child (widget_class, NautilusViewControls, view_toggle_group);
    gtk_widget_class_bind_template_callback (widget_class, on_view_button_toggled);
}

static void
nautilus_view_controls_init (NautilusViewControls *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));
    g_signal_connect_object (nautilus_list_view_preferences,
                             "changed::" NAUTILUS_PREFERENCES_LIST_VIEW_USE_TREE,
                             G_CALLBACK (on_tree_preference_changed), self, 0);

    g_autoptr (GtkBuilder) builder =
        gtk_builder_new_from_resource ("/org/gnome/nautilus/menu/nautilus-toolbar-view-menu.ui");
    g_autoptr (GMenuItem) sort_item = g_menu_item_new_from_model (self->view_menu, SORT_ITEM_POSITION);

    self->sort_section = G_MENU_MODEL (g_object_ref (gtk_builder_get_object (builder, "sort_section")));
    g_menu_item_set_section (sort_item, self->sort_section);
    g_menu_insert_item (G_MENU (self->view_menu), SORT_ITEM_POSITION, sort_item);
}
