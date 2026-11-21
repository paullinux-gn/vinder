/*
 * SPDX-FileCopyrightText: 2009 Red Hat, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "nautilus-metadata.h"
#include "nautilus-file.h"
#include <glib.h>

static char *used_metadata_names[] =
{
    NAUTILUS_METADATA_KEY_ICON_VIEW_SORT_BY,
    NAUTILUS_METADATA_KEY_ICON_VIEW_SORT_REVERSED,
    NAUTILUS_METADATA_KEY_LIST_VIEW_SORT_COLUMN,
    NAUTILUS_METADATA_KEY_LIST_VIEW_SORT_REVERSED,
    NAUTILUS_METADATA_KEY_LIST_VIEW_VISIBLE_COLUMNS,
    NAUTILUS_METADATA_KEY_LIST_VIEW_COLUMN_ORDER,
    NAUTILUS_METADATA_KEY_CUSTOM_ICON,
    NAUTILUS_METADATA_KEY_CUSTOM_ICON_NAME,
    NAUTILUS_METADATA_KEY_EMBLEMS,
    NAUTILUS_METADATA_KEY_VINDER_VIEW_MODE,
    NULL
};

guint
nautilus_metadata_get_id (const char *metadata)
{
    static GHashTable *hash;
    int i;

    if (hash == NULL)
    {
        hash = g_hash_table_new (g_str_hash, g_str_equal);
        for (i = 0; used_metadata_names[i] != NULL; i++)
        {
            g_hash_table_insert (hash,
                                 used_metadata_names[i],
                                 GINT_TO_POINTER (i + 1));
        }
    }

    return GPOINTER_TO_INT (g_hash_table_lookup (hash, metadata));
}

/* Make a user's selection available while the asynchronous metadata write runs. */
const char *
vinder_folder_get_view_mode (NautilusFile *folder)
{
    const char *mode = g_object_get_data (G_OBJECT (folder), NAUTILUS_METADATA_KEY_VINDER_VIEW_MODE);

    return mode != NULL ? mode : nautilus_file_get_metadata (folder, NAUTILUS_METADATA_KEY_VINDER_VIEW_MODE);
}

void
vinder_folder_set_view_mode (NautilusFile *folder,
                             const char   *mode)
{
    g_object_set_data_full (G_OBJECT (folder), NAUTILUS_METADATA_KEY_VINDER_VIEW_MODE,
                            g_strdup (mode), g_free);
    nautilus_file_set_metadata (folder, NAUTILUS_METADATA_KEY_VINDER_VIEW_MODE,
                                NAUTILUS_METADATA_STRING, (gpointer) mode);
}
