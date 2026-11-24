/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <glib.h>
#include <string.h>

#include "vinder-blur-css.h"

static void
assert_contains (const char *css,
                 const char *expected)
{
    if (strstr (css, expected) == NULL)
    {
        g_test_fail_printf ("Missing in the stylesheet: %s", expected);
    }
}

static void
assert_lacks (const char *css,
              const char *unexpected)
{
    if (strstr (css, unexpected) != NULL)
    {
        g_test_fail_printf ("Unexpectedly in the stylesheet: %s", unexpected);
    }
}

static void
test_default_sizes_keep_the_old_layout (void)
{
    g_autofree char *css = vinder_blur_css_build (78, 30, TRUE,
                                                  VINDER_DEFAULT_TOP_BAR_HEIGHT,
                                                  VINDER_DEFAULT_COLUMN_HEADER_HEIGHT);

    assert_contains (css, "gridview { padding-top: 66px; }");
    assert_contains (css, "margin-top: -112px;");
    assert_contains (css, "padding-top: calc(112px + 16px);");
    assert_contains (css, "min-height: 84px;");
    assert_contains (css, "transparent 40px,");
    assert_contains (css, "var(--vinder-header-tint) 48px,");
    assert_contains (css, "55%, transparent) 60px,");
}

static void
test_room_follows_the_real_sizes (void)
{
    /* Large text: taller toolbar and column header. */
    g_autofree char *css = vinder_blur_css_build (78, 30, TRUE, 90, 50);

    assert_contains (css, "gridview { padding-top: 90px; }");
    assert_contains (css, "margin-top: -140px;");
    assert_contains (css, "padding-top: calc(140px + 16px);");
    assert_contains (css, "min-height: 108px;");
    assert_contains (css, "transparent 64px,");
    assert_contains (css, "var(--vinder-header-tint) 72px,");
    assert_contains (css, "55%, transparent) 84px,");

    /* None of the old fixed numbers are left behind. */
    assert_lacks (css, "padding-top: 66px");
    assert_lacks (css, "-112px");
}

static void
test_nonsense_sizes_do_not_break_the_gradient (void)
{
    g_autofree char *css = vinder_blur_css_build (78, 30, TRUE, 0, -5);

    /* A toolbar cannot be smaller than 32px, so no stop goes negative. */
    assert_contains (css, "transparent 6px,");
    assert_contains (css, "var(--vinder-header-tint) 14px,");
    assert_contains (css, "margin-top: -32px;");
    assert_lacks (css, "transparent -");
    assert_lacks (css, "header-tint) -");
}

static void
test_header_is_capped_but_other_surfaces_are_not (void)
{
    g_autofree char *css = vinder_blur_css_build (100, 40, TRUE,
                                                  VINDER_DEFAULT_TOP_BAR_HEIGHT,
                                                  VINDER_DEFAULT_COLUMN_HEADER_HEIGHT);

    /* The toolbar and column headers stop at the documented limits... */
    assert_contains (css, "var(--headerbar-bg-color) 62%");
    assert_contains (css, "blur(18px);");
}

static void
test_header_cap_values_match_the_documented_ones (void)
{
    g_autofree char *css = vinder_blur_css_build (100, 40, FALSE,
                                                  VINDER_DEFAULT_TOP_BAR_HEIGHT,
                                                  VINDER_DEFAULT_COLUMN_HEADER_HEIGHT);
    g_autofree char *capped_opacity = g_strdup_printf ("var(--headerbar-bg-color) %d%%", VINDER_HEADER_MAX_OPACITY);
    g_autofree char *capped_blur = g_strdup_printf ("blur(%dpx);", VINDER_HEADER_MAX_BLUR);

    assert_contains (css, capped_opacity);
    assert_contains (css, capped_blur);

    /* ...while the sidebar and popups use the full slider values. */
    assert_contains (css, "var(--sidebar-bg-color) 100%");
    assert_contains (css, "blur(40px);");
}

static void
test_below_the_cap_the_sliders_apply_as_they_are (void)
{
    g_autofree char *css = vinder_blur_css_build (40, 10, FALSE,
                                                  VINDER_DEFAULT_TOP_BAR_HEIGHT,
                                                  VINDER_DEFAULT_COLUMN_HEADER_HEIGHT);

    assert_contains (css, "var(--headerbar-bg-color) 40%");
    assert_contains (css, "blur(10px);");
    assert_lacks (css, "62%");
}

static void
test_menu_blur_can_be_switched_off (void)
{
    g_autofree char *with_menus = vinder_blur_css_build (78, 30, TRUE, 66, 46);
    g_autofree char *without_menus = vinder_blur_css_build (78, 30, FALSE, 66, 46);

    assert_contains (with_menus, "popover > contents");
    assert_lacks (without_menus, "popover > contents");
}

static void
test_every_preset_recognises_itself (void)
{
    for (guint i = 0; i < vinder_blur_preset_count (); i++)
    {
        const VinderBlurPreset *preset = vinder_blur_preset_get (i);

        g_assert_nonnull (preset);
        g_assert_cmpstr (vinder_blur_preset_match (preset->blur, preset->opacity, preset->strength),
                         ==, preset->name);
        g_assert_true (vinder_blur_preset_find (preset->name) == preset);
    }

    g_assert_null (vinder_blur_preset_get (vinder_blur_preset_count ()));
    g_assert_null (vinder_blur_preset_find ("bogus"));
    g_assert_null (vinder_blur_preset_find (NULL));
}

static void
test_presets_stay_inside_the_slider_ranges (void)
{
    g_autoptr (GHashTable) names = g_hash_table_new (g_str_hash, g_str_equal);

    g_assert_cmpuint (vinder_blur_preset_count (), ==, 4);

    for (guint i = 0; i < vinder_blur_preset_count (); i++)
    {
        const VinderBlurPreset *preset = vinder_blur_preset_get (i);

        /* The sliders go from 0 to 100 (opacity) and 0 to 40 (strength). */
        g_assert_cmpfloat (preset->opacity, >=, 0);
        g_assert_cmpfloat (preset->opacity, <=, 100);
        g_assert_cmpfloat (preset->strength, >=, 0);
        g_assert_cmpfloat (preset->strength, <=, 40);
        g_assert_true (g_hash_table_add (names, (gpointer) preset->name));
    }
}

static void
test_off_ignores_the_sliders (void)
{
    g_assert_cmpstr (vinder_blur_preset_match (FALSE, 78, 30), ==, "off");
    g_assert_cmpstr (vinder_blur_preset_match (FALSE, 12, 3), ==, "off");

    /* Blur on never counts as "off". */
    g_assert_cmpstr (vinder_blur_preset_match (TRUE, 78, 30), !=, "off");
}

static void
test_a_custom_mix_matches_nothing (void)
{
    g_assert_null (vinder_blur_preset_match (TRUE, 70, 30));
    g_assert_null (vinder_blur_preset_match (TRUE, 78, 31));
    g_assert_null (vinder_blur_preset_match (TRUE, 0, 0));
}

static void
test_rounding_is_forgiving (void)
{
    /* The sliders are whole numbers, but settings are doubles. */
    g_assert_cmpstr (vinder_blur_preset_match (TRUE, 78.4, 29.6), ==, "glass");
    g_assert_cmpstr (vinder_blur_preset_match (TRUE, 91.6, 12.2), ==, "subtle");
}

static void
test_glass_is_what_vinder_always_used (void)
{
    /* Switching blur on used to give exactly these values. */
    const VinderBlurPreset *glass = vinder_blur_preset_find ("glass");

    g_assert_nonnull (glass);
    g_assert_true (glass->blur);
    g_assert_cmpfloat (glass->opacity, ==, 78);
    g_assert_cmpfloat (glass->strength, ==, 30);
}

int
main (int   argc,
      char *argv[])
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/vinder-blur-css/default-sizes-keep-the-old-layout",
                     test_default_sizes_keep_the_old_layout);
    g_test_add_func ("/vinder-blur-css/room-follows-the-real-sizes",
                     test_room_follows_the_real_sizes);
    g_test_add_func ("/vinder-blur-css/nonsense-sizes",
                     test_nonsense_sizes_do_not_break_the_gradient);
    g_test_add_func ("/vinder-blur-css/header-capped-others-not",
                     test_header_is_capped_but_other_surfaces_are_not);
    g_test_add_func ("/vinder-blur-css/cap-values-match-documented",
                     test_header_cap_values_match_the_documented_ones);
    g_test_add_func ("/vinder-blur-css/below-the-cap",
                     test_below_the_cap_the_sliders_apply_as_they_are);
    g_test_add_func ("/vinder-blur-css/menu-blur-switch",
                     test_menu_blur_can_be_switched_off);
    g_test_add_func ("/vinder-blur-presets/recognise-themselves",
                     test_every_preset_recognises_itself);
    g_test_add_func ("/vinder-blur-presets/inside-slider-ranges",
                     test_presets_stay_inside_the_slider_ranges);
    g_test_add_func ("/vinder-blur-presets/off-ignores-sliders",
                     test_off_ignores_the_sliders);
    g_test_add_func ("/vinder-blur-presets/custom-matches-nothing",
                     test_a_custom_mix_matches_nothing);
    g_test_add_func ("/vinder-blur-presets/rounding",
                     test_rounding_is_forgiving);
    g_test_add_func ("/vinder-blur-presets/glass-is-the-old-default",
                     test_glass_is_what_vinder_always_used);

    return g_test_run ();
}
