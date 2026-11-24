/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "vinder-thumbnail-preparation.h"
#include <gtk/gtk.h>

#include "nautilus-global-preferences.h"
#include "nautilus-thumbnails.h"
#include "vinder-thumbnail-scheduler.h"

/* Thumbnails are prepared once the user has left Vinder alone for this long. */
#define IDLE_MS 5000

#define FOLDERS_KEY "vinder-thumbnail-folders"

#define THUMBNAIL_ATTRIBUTES G_FILE_ATTRIBUTE_STANDARD_NAME "," \
    G_FILE_ATTRIBUTE_STANDARD_TYPE "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE "," \
    G_FILE_ATTRIBUTE_STANDARD_SIZE "," G_FILE_ATTRIBUTE_TIME_MODIFIED "," \
    G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID "," G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN

typedef struct
{
    VinderThumbnailScheduler *scheduler;
    GPowerProfileMonitor *power_monitor;
} Preparation;

static gboolean
start_item (GFile              *file,
            GFileInfo          *info,
            GCancellable       *cancellable,
            GAsyncReadyCallback callback,
            gpointer            callback_data,
            gpointer            user_data)
{
    const char *content_type = g_file_info_get_content_type (info);
    guint64 size_limit;

    if (g_file_info_get_file_type (info) != G_FILE_TYPE_REGULAR ||
        g_file_info_get_is_hidden (info) || content_type == NULL ||
        !g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_TIME_MODIFIED) ||
        g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID))
    {
        return FALSE;
    }

    size_limit = g_settings_get_uint64 (nautilus_preferences,
                                        NAUTILUS_PREFERENCES_FILE_THUMBNAIL_LIMIT) * 1000000;

    g_autofree char *mime_type = g_content_type_get_mime_type (content_type);
    g_autofree char *uri = g_file_get_uri (file);
    time_t mtime = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_MODIFIED);

    if (mime_type == NULL ||
        ((guint64) g_file_info_get_size (info) > size_limit &&
         nautilus_thumbnail_is_mimetype_limited_by_size (mime_type)) ||
        !nautilus_can_thumbnail (uri, mime_type, mtime))
    {
        return FALSE;
    }

    nautilus_create_thumbnail_async (uri, mime_type, mtime, cancellable, callback, callback_data);
    return TRUE;
}

/* ---- when to work ---- */

static void
update_paused (Preparation *self)
{
    gboolean thumbnails_off =
        g_settings_get_enum (nautilus_preferences, NAUTILUS_PREFERENCES_SHOW_FILE_THUMBNAILS) ==
        NAUTILUS_SPEED_TRADEOFF_NEVER;
    gboolean power_saver = g_power_profile_monitor_get_power_saver_enabled (self->power_monitor);

    /* Turning thumbnails back on, or leaving power saver, lets the work carry on. */
    vinder_thumbnail_scheduler_set_paused (self->scheduler, thumbnails_off || power_saver);
}

static void
on_power_saver_changed (GObject    *monitor,
                        GParamSpec *pspec,
                        gpointer    user_data)
{
    update_paused (user_data);
}

static void
on_thumbnails_setting_changed (GSettings  *settings,
                               const char *key,
                               gpointer    user_data)
{
    update_paused (user_data);
}

static gboolean
on_input_event_cb (GtkEventControllerLegacy *controller,
                   GdkEvent                 *event,
                   gpointer                  user_data)
{
    vinder_thumbnail_scheduler_notify_activity (user_data);

    /* This capture-phase controller only records activity; never consume input. */
    return GDK_EVENT_PROPAGATE;
}

GtkEventController *
vinder_thumbnail_preparation_activity_controller_new (VinderThumbnailScheduler *scheduler)
{
    GtkEventController *controller = gtk_event_controller_legacy_new ();

    gtk_event_controller_set_propagation_phase (controller, GTK_PHASE_CAPTURE);
    g_signal_connect_object (controller, "event", G_CALLBACK (on_input_event_cb), scheduler, 0);

    return controller;
}

/* Activity only counts when it happens in one of Vinder's own windows. */
static void
watch_window_input (GtkApplication *application,
                    GtkWindow      *window,
                    gpointer        user_data)
{
    gtk_widget_add_controller (GTK_WIDGET (window),
                               vinder_thumbnail_preparation_activity_controller_new (user_data));
}

/* ---- which folders ---- */

static void
add_folder (GPtrArray  *paths,
            const char *path)
{
    g_autofree char *canonical = NULL;

    if (path == NULL || !g_path_is_absolute (path))
    {
        return;
    }

    canonical = g_canonicalize_filename (path, NULL);
    if (g_str_equal (canonical, G_DIR_SEPARATOR_S) ||
        g_str_equal (canonical, g_get_home_dir ()))
    {
        return;
    }

    for (guint i = 0; i < paths->len; i++)
    {
        if (g_str_equal (g_ptr_array_index (paths, i), canonical))
        {
            return;
        }
    }

    g_ptr_array_add (paths, g_steal_pointer (&canonical));
}

static void
update_folders (Preparation *self)
{
    g_auto (GStrv) configured = g_settings_get_strv (nautilus_preferences, FOLDERS_KEY);
    g_autoptr (GPtrArray) paths = g_ptr_array_new_with_free_func (g_free);

    for (guint i = 0; configured[i] != NULL; i++)
    {
        add_folder (paths, configured[i]);
    }

    if (g_strv_length (configured) == 0)
    {
        add_folder (paths, g_get_user_special_dir (G_USER_DIRECTORY_PICTURES));
    }

    g_ptr_array_add (paths, NULL);
    vinder_thumbnail_scheduler_set_folders (self->scheduler, (const char * const *) paths->pdata);
}

static void
on_folders_setting_changed (GSettings  *settings,
                            const char *key,
                            gpointer    user_data)
{
    update_folders (user_data);
}

/* ---- setup and teardown ---- */

static void
on_shutdown (GApplication *application,
             gpointer      user_data)
{
    vinder_thumbnail_scheduler_stop (user_data);
}

static void
preparation_free (Preparation *self)
{
    g_signal_handlers_disconnect_by_data (nautilus_preferences, self);
    g_signal_handlers_disconnect_by_data (self->power_monitor, self);
    g_clear_object (&self->power_monitor);

    vinder_thumbnail_scheduler_stop (self->scheduler);
    g_clear_object (&self->scheduler);
    g_free (self);
}

void
vinder_thumbnail_preparation_start (GApplication *application)
{
    Preparation *self = g_new0 (Preparation, 1);

    self->scheduler = vinder_thumbnail_scheduler_new (THUMBNAIL_ATTRIBUTES, IDLE_MS,
                                                      start_item, NULL);
    self->power_monitor = g_power_profile_monitor_dup_default ();

    g_signal_connect (self->power_monitor, "notify::power-saver-enabled",
                      G_CALLBACK (on_power_saver_changed), self);
    g_signal_connect (nautilus_preferences, "changed::" NAUTILUS_PREFERENCES_SHOW_FILE_THUMBNAILS,
                      G_CALLBACK (on_thumbnails_setting_changed), self);
    g_signal_connect (nautilus_preferences, "changed::" FOLDERS_KEY,
                      G_CALLBACK (on_folders_setting_changed), self);

    if (GTK_IS_APPLICATION (application))
    {
        for (GList *l = gtk_application_get_windows (GTK_APPLICATION (application)); l != NULL; l = l->next)
        {
            watch_window_input (GTK_APPLICATION (application), l->data, self->scheduler);
        }

        g_signal_connect_object (application, "window-added",
                                 G_CALLBACK (watch_window_input), self->scheduler, 0);
    }

    g_signal_connect_object (application, "shutdown",
                             G_CALLBACK (on_shutdown), self->scheduler, 0);

    update_paused (self);
    update_folders (self);

    g_object_set_data_full (G_OBJECT (application), "vinder-thumbnail-preparation",
                            self, (GDestroyNotify) preparation_free);
}
