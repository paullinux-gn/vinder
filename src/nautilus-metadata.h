/*
 * SPDX-FileCopyrightText: 2000 Eazel, Inc.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Author: John Sullivan <sullivan@eazel.com>
 */

#pragma once

/* Keys for getting/setting Nautilus metadata. All metadata used in Nautilus
 * should define its key here, so we can keep track of the whole set easily.
 * Any updates here needs to be added in nautilus-metadata.c too.
 */

#include <glib.h>
#include "nautilus-types.h"

/* Per-file */

#define NAUTILUS_METADATA_KEY_VINDER_VIEW_MODE "vinder-view-mode"

#define NAUTILUS_METADATA_KEY_ICON_VIEW_SORT_BY          	"nautilus-icon-view-sort-by"
#define NAUTILUS_METADATA_KEY_ICON_VIEW_SORT_REVERSED    	"nautilus-icon-view-sort-reversed"

#define NAUTILUS_METADATA_KEY_LIST_VIEW_SORT_COLUMN      	"nautilus-list-view-sort-column"
#define NAUTILUS_METADATA_KEY_LIST_VIEW_SORT_REVERSED    	"nautilus-list-view-sort-reversed"
#define NAUTILUS_METADATA_KEY_LIST_VIEW_VISIBLE_COLUMNS    	"nautilus-list-view-visible-columns"
#define NAUTILUS_METADATA_KEY_LIST_VIEW_COLUMN_ORDER    	"nautilus-list-view-column-order"

#define NAUTILUS_METADATA_KEY_CUSTOM_ICON                	"custom-icon"
#define NAUTILUS_METADATA_KEY_CUSTOM_ICON_NAME                	"custom-icon-name"
#define NAUTILUS_METADATA_KEY_EMBLEMS				"emblems"

guint nautilus_metadata_get_id (const char *metadata);

const char *vinder_folder_get_view_mode (NautilusFile *folder);
void vinder_folder_set_view_mode (NautilusFile *folder, const char *mode);
