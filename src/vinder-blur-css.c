/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "vinder-blur-css.h"

#include <math.h>

/* Space between the column header and the first row of the list. */
#define LIST_ROW_GAP 16

/* The soft edge under the toolbar fades out this far below the toolbar's bottom
 * edge, and starts to fade in these distances above it. */
#define FADE_BELOW 18
#define FADE_START_ABOVE 26
#define FADE_TINT_ABOVE 18
#define FADE_HALF_ABOVE 6

/* Anything smaller than this cannot be a real toolbar. */
#define MIN_TOP_BAR_HEIGHT 32

char *
vinder_blur_css_build (double   opacity,
                       double   strength,
                       gboolean blur_menus,
                       int      top_bar_height,
                       int      column_header_height)
{
    g_autofree char *menu_css = NULL;
    int top = MAX (top_bar_height, MIN_TOP_BAR_HEIGHT);
    int list_offset = top + MAX (column_header_height, 0);

    if (blur_menus)
    {
        menu_css = g_strdup_printf (
            "  .vinder-window popover > contents,"
            "  .vinder-window .floating-bar,"
            "  .vinder-window popover > arrow {"
            "    backdrop-filter: blur(%dpx);"
            "    background-color: color-mix(in srgb, var(--popover-bg-color) %d%%, transparent);"
            "  }", (int) strength, (int) opacity);
    }

    return g_strdup_printf (
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
        "  .vinder-content.scroll-animation .nautilus-grid-view gridview { padding-top: %dpx; }"
        "  .vinder-content.scroll-animation .nautilus-list-view columnview > listview {"
        "    margin-top: -%dpx;"
        "    padding-top: calc(%dpx + %dpx);"
        "  }"
        "  .vinder-content.scroll-animation .nautilus-grid-view scrolledwindow > undershoot.top,"
        "  .vinder-content.scroll-animation .nautilus-list-view scrolledwindow > undershoot.top {"
        "    min-height: %dpx;"
        "    background: linear-gradient(to bottom,"
        "      transparent %dpx,"
        "      var(--vinder-header-tint) %dpx,"
        "      color-mix(in srgb, var(--vinder-header-tint) 55%%, transparent) %dpx,"
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
        "}",
        MIN ((int) opacity, VINDER_HEADER_MAX_OPACITY),
        MIN ((int) strength, VINDER_HEADER_MAX_BLUR),
        top,
        list_offset,
        list_offset, LIST_ROW_GAP,
        top + FADE_BELOW,
        top - FADE_START_ABOVE, top - FADE_TINT_ABOVE, top - FADE_HALF_ABOVE,
        (int) strength, (int) opacity,
        (int) opacity,
        (int) strength, (int) opacity,
        menu_css != NULL ? menu_css : "");
}

/* "glass" is what Vinder has always used when blur is switched on. */
static const VinderBlurPreset presets[] =
{
    { "off", FALSE, 0, 0 },
    { "subtle", TRUE, 92, 12 },
    { "glass", TRUE, 78, 30 },
    { "frosted", TRUE, 60, 40 },
};

guint
vinder_blur_preset_count (void)
{
    return G_N_ELEMENTS (presets);
}

const VinderBlurPreset *
vinder_blur_preset_get (guint index)
{
    return index < G_N_ELEMENTS (presets) ? &presets[index] : NULL;
}

const VinderBlurPreset *
vinder_blur_preset_find (const char *name)
{
    for (guint i = 0; name != NULL && i < G_N_ELEMENTS (presets); i++)
    {
        if (g_str_equal (presets[i].name, name))
        {
            return &presets[i];
        }
    }

    return NULL;
}

const char *
vinder_blur_preset_match (gboolean blur,
                          double   opacity,
                          double   strength)
{
    for (guint i = 0; i < G_N_ELEMENTS (presets); i++)
    {
        if (presets[i].blur != !!blur)
        {
            continue;
        }

        /* Without blur only "off" applies, whatever the sliders say. */
        if (!blur ||
            (round (opacity) == presets[i].opacity && round (strength) == presets[i].strength))
        {
            return presets[i].name;
        }
    }

    return NULL;
}
