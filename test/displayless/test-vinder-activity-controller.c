/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <gtk/gtk.h>

#include "vinder-thumbnail-preparation.h"
#include "vinder-thumbnail-scheduler.h"

static gboolean
never_called (GFile              *file,
              GFileInfo          *info,
              GCancellable       *cancellable,
              GAsyncReadyCallback callback,
              gpointer            callback_data,
              gpointer            user_data)
{
    return FALSE;
}

static void
test_input_events_are_never_swallowed (void)
{
    VinderThumbnailScheduler *scheduler =
        vinder_thumbnail_scheduler_new ("standard::name", 5000, never_called, NULL);
    GtkEventController *controller =
        vinder_thumbnail_preparation_activity_controller_new (scheduler);
    gboolean handled = TRUE;

    g_assert_cmpint (gtk_event_controller_get_propagation_phase (controller), ==, GTK_PHASE_CAPTURE);

    /* The event data is unused; verify the handler always lets it propagate. */
    for (guint i = 0; i < 100; i++)
    {
        handled = TRUE;
        g_signal_emit_by_name (controller, "event", NULL, &handled);
        g_assert_false (handled);
    }

    g_object_unref (controller);
    vinder_thumbnail_scheduler_stop (scheduler);
    g_object_unref (scheduler);
}

int
main (int   argc,
      char *argv[])
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/vinder-activity-controller/input-events-are-never-swallowed",
                     test_input_events_are_never_swallowed);

    return g_test_run ();
}
