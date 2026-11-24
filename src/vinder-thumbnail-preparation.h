/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <gio/gio.h>
#include <gtk/gtk.h>

#include "vinder-thumbnail-scheduler.h"

void vinder_thumbnail_preparation_start (GApplication *application);

/* Construct the capture-phase controller used to note window activity. */
GtkEventController *vinder_thumbnail_preparation_activity_controller_new (VinderThumbnailScheduler *scheduler);
