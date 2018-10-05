#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void append_to_log (char c);

#ifdef _WIN32

#include <windows.h>

BOOL WINAPI
DllMain (HINSTANCE instance, DWORD reason, LPVOID reserved)
{
  (void) instance;
  (void) reserved;

  switch (reason)
  {
    case DLL_PROCESS_ATTACH:
      append_to_log ('>');
      break;
    case DLL_PROCESS_DETACH:
      append_to_log ('<');
      break;
    default:
      break;
  }

  return TRUE;
}

#else

__attribute__ ((constructor)) static void
on_load (void)
{
  append_to_log ('>');
}

__attribute__ ((destructor)) static void
on_unload (void)
{
  append_to_log ('<');
}

#endif

void
cateyes_agent_main (const char * data)
{
  append_to_log ('m');

  if (strlen (data) > 0)
  {
    int exit_code = atoi (data);
    exit (exit_code);
  }
}

static void
append_to_log (char c)
{
#ifdef _WIN32
  wchar_t * path;
  HANDLE file;
  BOOL written;

  path = _wgetenv (L"CATEYES_LABRAT_LOGFILE");
  assert (path != NULL);

  file = CreateFileW (path, FILE_APPEND_DATA, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  assert (file != INVALID_HANDLE_VALUE);

  written = WriteFile (file, &c, sizeof (c), NULL, NULL);
  assert (written);

  CloseHandle (file);
#else
  FILE * f;

  f = fopen (getenv ("CATEYES_LABRAT_LOGFILE"), "ab");
  assert (f != NULL);
  fwrite (&c, 1, 1, f);
  fclose (f);
#endif
}
