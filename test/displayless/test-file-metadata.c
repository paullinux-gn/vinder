#include <glib.h>

#include <nautilus-file.h>
#include <nautilus-file-utilities.h>
#include <nautilus-metadata.h>

static const char *TEST_FILE = "file:///etc/passwd";
static const char *KEY_BOOL = NAUTILUS_METADATA_KEY_ICON_VIEW_SORT_REVERSED;
static const char *KEY_STR = NAUTILUS_METADATA_KEY_ICON_VIEW_SORT_BY;

static void
test_file_metadata_bool_set_true (void)
{
    g_autoptr (NautilusFile) file = nautilus_file_get_by_uri (TEST_FILE);
    nautilus_file_set_boolean_metadata (file, KEY_BOOL, TRUE);
    g_assert_true (nautilus_file_get_boolean_metadata (file, KEY_BOOL, FALSE));
}

static void
test_file_metadata_bool_set_false (void)
{
    g_autoptr (NautilusFile) file = nautilus_file_get_by_uri (TEST_FILE);
    nautilus_file_set_boolean_metadata (file, KEY_BOOL, FALSE);
    g_assert_false (nautilus_file_get_boolean_metadata (file, KEY_BOOL, TRUE));
}

static void
test_file_metadata_str_set (void)
{
    g_autoptr (NautilusFile) file = nautilus_file_get_by_uri (TEST_FILE);

    nautilus_file_set_metadata (file, KEY_STR, NAUTILUS_METADATA_STRING, "value");

    g_assert_cmpstr (nautilus_file_get_metadata (file, KEY_STR), ==, "value");
}

static void
test_folder_view_modes (void)
{
    g_autoptr (NautilusFile) pictures = nautilus_file_get_by_uri ("file:///tmp/vinder-test-pictures");
    g_autoptr (NautilusFile) documents = nautilus_file_get_by_uri ("file:///tmp/vinder-test-documents");

    /* Existing metadata must work before a folder is changed in this session. */
    nautilus_file_set_metadata (pictures, NAUTILUS_METADATA_KEY_VINDER_VIEW_MODE,
                                NAUTILUS_METADATA_STRING, "grid");
    g_assert_cmpstr (vinder_folder_get_view_mode (pictures), ==, "grid");

    vinder_folder_set_view_mode (documents, "tree");
    vinder_folder_set_view_mode (pictures, "list");
    g_assert_cmpstr (vinder_folder_get_view_mode (pictures), ==, "list");
    g_assert_cmpstr (vinder_folder_get_view_mode (documents), ==, "tree");

    vinder_folder_set_view_mode (pictures, "grid");
    g_assert_cmpstr (nautilus_file_get_metadata (pictures, NAUTILUS_METADATA_KEY_VINDER_VIEW_MODE),
                     ==, "grid");
    g_assert_cmpstr (vinder_folder_get_view_mode (documents), ==, "tree");
}

int
main (int   argc,
      char *argv[])
{
    g_test_init (&argc, &argv, NULL);
    g_test_set_nonfatal_assertions ();
    nautilus_ensure_extension_points ();

    g_test_add_func ("/file-metadata-set-bool/true",
                     test_file_metadata_bool_set_true);
    g_test_add_func ("/file-metadata-set-bool/false",
                     test_file_metadata_bool_set_false);
    g_test_add_func ("/file-metadata-str-set/default",
                     test_file_metadata_str_set);

    g_test_add_func ("/folder-view-metadata/independent-folders", test_folder_view_modes);

    return g_test_run ();
}
