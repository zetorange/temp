#include "cateyes-core.h"

#include "icon-helpers.h"

#include <psapi.h>

#define DRIVE_STRINGS_MAX_LENGTH     (512)

static gboolean get_process_filename (HANDLE process, WCHAR * name, DWORD name_capacity);

void
cateyes_system_get_frontmost_application (CateyesHostApplicationInfo * result, GError ** error)
{
  (void) result;

  g_set_error (error,
      CATEYES_ERROR,
      CATEYES_ERROR_NOT_SUPPORTED,
      "Not implemented");
}

CateyesHostApplicationInfo *
cateyes_system_enumerate_applications (int * result_length)
{
  *result_length = 0;

  return NULL;
}

CateyesHostProcessInfo *
cateyes_system_enumerate_processes (int * result_length)
{
  GArray * processes;
  DWORD * pids = NULL;
  DWORD size = 64 * sizeof (DWORD);
  DWORD bytes_returned;
  guint i;

  processes = g_array_new (FALSE, FALSE, sizeof (CateyesHostProcessInfo));

  do
  {
    size *= 2;
    pids = (DWORD *) g_realloc (pids, size);
    if (!EnumProcesses (pids, size, &bytes_returned))
      bytes_returned = 0;
  }
  while (bytes_returned == size);

  for (i = 0; i != bytes_returned / sizeof (DWORD); i++)
  {
    HANDLE handle;

    handle = OpenProcess (PROCESS_QUERY_INFORMATION, FALSE, pids[i]);
    if (handle != NULL)
    {
      WCHAR name_utf16[MAX_PATH];
      DWORD name_length = MAX_PATH;

      if (get_process_filename (handle, name_utf16, name_length))
      {
        gchar * name, * tmp;
        CateyesHostProcessInfo * process_info;
        CateyesImageData * small_icon, * large_icon;

        name = g_utf16_to_utf8 ((gunichar2 *) name_utf16, -1, NULL, NULL, NULL);
        tmp = g_path_get_basename (name);
        g_free (name);
        name = tmp;

        small_icon = _cateyes_image_data_from_process_or_file (pids[i], name_utf16, CATEYES_ICON_SMALL);
        large_icon = _cateyes_image_data_from_process_or_file (pids[i], name_utf16, CATEYES_ICON_LARGE);

        g_array_set_size (processes, processes->len + 1);
        process_info = &g_array_index (processes, CateyesHostProcessInfo, processes->len - 1);
        cateyes_host_process_info_init (process_info, pids[i], name, small_icon, large_icon);

        cateyes_image_data_free (large_icon);
        cateyes_image_data_free (small_icon);

        g_free (name);
      }

      CloseHandle (handle);
    }
  }

  g_free (pids);

  *result_length = processes->len;

  return (CateyesHostProcessInfo *) g_array_free (processes, FALSE);
}

void
cateyes_system_kill (guint pid)
{
  HANDLE process;

  process = OpenProcess (PROCESS_TERMINATE, FALSE, pid);
  if (process != NULL)
  {
    TerminateProcess (process, 0xdeadbeef);
    CloseHandle (process);
  }
}

gchar *
cateyes_temporary_directory_get_system_tmp (void)
{
  return g_strdup (g_get_tmp_dir ());
}

static gboolean
get_process_filename (HANDLE process, WCHAR * name, DWORD name_capacity)
{
  gsize name_length;
  WCHAR drive_strings[DRIVE_STRINGS_MAX_LENGTH];
  WCHAR *drive;

  if (GetProcessImageFileName (process, name, name_capacity) == 0)
    return FALSE;
  name_length = wcslen (name);

  drive_strings[0] = L'\0';
  drive_strings[DRIVE_STRINGS_MAX_LENGTH - 1] = L'\0';
  GetLogicalDriveStringsW (DRIVE_STRINGS_MAX_LENGTH - 1, drive_strings);
  for (drive = drive_strings; *drive != '\0'; drive += wcslen (drive) + 1)
  {
    WCHAR device_name[3];
    WCHAR mapping_strings[MAX_PATH];
    WCHAR *mapping;
    gsize mapping_length;

    wcsncpy_s (device_name, 3, drive, 2);

    mapping_strings[0] = '\0';
    mapping_strings[MAX_PATH - 1] = '\0';
    QueryDosDevice (device_name, mapping_strings, MAX_PATH - 1);
    for (mapping = mapping_strings; *mapping != '\0'; mapping += mapping_length + 1)
    {
      mapping_length = wcslen (mapping);

      if (mapping_length > name_length)
        continue;

      if (wcsncmp (name, mapping, mapping_length) == 0)
      {
        wcscpy_s (name, 3, device_name);
        memmove (name + 2, name + mapping_length, (name_length - mapping_length + 1) * sizeof (WCHAR));
        return TRUE;
      }
    }
  }

  return FALSE;
}
