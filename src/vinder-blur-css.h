/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include <glib.h>

G_BEGIN_DECLS

/* With Scroll Animation on, the toolbar and the column headers sit over scrolling
 * content, so they are kept lighter than the other surfaces: whatever the sliders
 * say, they never go above these values. The Interface dialog tells the user. */
#define VINDER_HEADER_MAX_OPACITY 62
#define VINDER_HEADER_MAX_BLUR 18

/* Sizes used until the real ones are known (the default font size). */
#define VINDER_DEFAULT_TOP_BAR_HEIGHT 66
#define VINDER_DEFAULT_COLUMN_HEADER_HEIGHT 46

/* Builds the stylesheet for Vinder's blur effect.
 *
 * @top_bar_height is the real height of the toolbar above the files and
 * @column_header_height the real height of the list view's column header. The
 * scroll animation lets files scroll under both, so the space reserved for them
 * has to follow their size (large text, other fonts...) instead of being fixed. */
char *vinder_blur_css_build (double   opacity,
                             double   strength,
                             gboolean blur_menus,
                             int      top_bar_height,
                             int      column_header_height);

/* Ready-made combinations of the blur settings, shown as one row in the Interface
 * dialog. "off" only turns the blur off and leaves the sliders where they are. */
typedef struct
{
    const char *name;
    gboolean blur;
    double opacity;
    double strength;
} VinderBlurPreset;

guint vinder_blur_preset_count (void);
const VinderBlurPreset *vinder_blur_preset_get (guint index);
const VinderBlurPreset *vinder_blur_preset_find (const char *name);

/* The name of the preset the current settings add up to, or NULL for a custom mix. */
const char *vinder_blur_preset_match (gboolean blur,
                                      double   opacity,
                                      double   strength);

G_END_DECLS
