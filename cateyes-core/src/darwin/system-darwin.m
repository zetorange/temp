#include "cateyes-core.h"

#include "icon-helpers.h"

#include <signal.h>
#include <unistd.h>
#include <sys/sysctl.h>

static struct kinfo_proc * cateyes_system_query_kinfo_procs (guint * count);

#ifdef HAVE_MACOS

# include <libproc.h>
# import <AppKit/AppKit.h>

static void extract_icons_from_image (NSImage * image, CateyesImageData * small_icon, CateyesImageData * large_icon);

#endif

#ifdef HAVE_IOS

# import "springboard.h"

static void extract_icons_from_identifier (NSString * identifier, CateyesImageData * small_icon, CateyesImageData * large_icon);

extern int proc_pidpath (int pid, void * buffer, uint32_t buffer_size);

#endif

#ifndef PROC_PIDPATHINFO_MAXSIZE
# define PROC_PIDPATHINFO_MAXSIZE (4 * MAXPATHLEN)
#endif

typedef struct _CateyesIconPair CateyesIconPair;

struct _CateyesIconPair
{
  CateyesImageData small_icon;
  CateyesImageData large_icon;
};

static void cateyes_icon_pair_free (CateyesIconPair * pair);

static GHashTable * icon_pair_by_identifier = NULL;

static void
cateyes_system_init (void)
{
  if (icon_pair_by_identifier == NULL)
  {
    icon_pair_by_identifier = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify) cateyes_icon_pair_free);
  }
}

void
cateyes_system_get_frontmost_application (CateyesHostApplicationInfo * result, GError ** error)
{
#ifdef HAVE_IOS
  NSAutoreleasePool * pool;
  CateyesSpringboardApi * api;
  NSString * identifier;

  cateyes_system_init ();

  pool = [[NSAutoreleasePool alloc] init];

  api = _cateyes_get_springboard_api ();

  identifier = api->SBSCopyFrontmostApplicationDisplayIdentifier ();
  if (identifier != nil)
  {
    NSString * name;
    struct kinfo_proc * entries;
    guint count, i;

    result->_identifier = g_strdup ([identifier UTF8String]);
    name = api->SBSCopyLocalizedApplicationNameForDisplayIdentifier (identifier);
    result->_name = g_strdup ([name UTF8String]);
    [name release];

    entries = cateyes_system_query_kinfo_procs (&count);
    for (result->_pid = 0, i = 0; result->_pid == 0 && i != count; i++)
    {
      guint pid = entries[i].kp_proc.p_pid;
      NSString * cur_identifier;

      cur_identifier = api->SBSCopyDisplayIdentifierForProcessID (pid);
      if (cur_identifier != nil)
      {
        if ([cur_identifier isEqualToString:identifier])
          result->_pid = pid;
        [cur_identifier release];
      }
    }
    g_free (entries);

    extract_icons_from_identifier (identifier, &result->_small_icon, &result->_large_icon);

    [identifier release];
  }
  else
  {
    result->_identifier = g_strdup ("");
    result->_name = g_strdup ("");
    result->_pid = 0;
    cateyes_image_data_init (&result->_small_icon, 0, 0, 0, "");
    cateyes_image_data_init (&result->_large_icon, 0, 0, 0, "");
  }

  [pool release];
#else
  g_set_error (error,
      CATEYES_ERROR,
      CATEYES_ERROR_NOT_SUPPORTED,
      "Not implemented");
#endif
}

CateyesHostApplicationInfo *
cateyes_system_enumerate_applications (int * result_length)
{
#ifdef HAVE_IOS
  NSAutoreleasePool * pool;
  CateyesSpringboardApi * api;
  GHashTable * pid_by_identifier;
  struct kinfo_proc * entries;
  NSArray * identifiers;
  guint count, i;
  CateyesHostApplicationInfo * result;

  cateyes_system_init ();

  pool = [[NSAutoreleasePool alloc] init];

  api = _cateyes_get_springboard_api ();

  pid_by_identifier = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  entries = cateyes_system_query_kinfo_procs (&count);

  for (i = 0; i != count; i++)
  {
    struct kinfo_proc * e = &entries[i];
    guint pid = e->kp_proc.p_pid;
    NSString * identifier;

    identifier = api->SBSCopyDisplayIdentifierForProcessID (pid);
    if (identifier != nil)
    {
      g_hash_table_insert (pid_by_identifier, g_strdup ([identifier UTF8String]), GUINT_TO_POINTER (pid));
      [identifier release];
    }
  }

  g_free (entries);

  identifiers = api->SBSCopyApplicationDisplayIdentifiers (NO, NO);

  count = [identifiers count];
  result = g_new0 (CateyesHostApplicationInfo, count);
  *result_length = count;

  for (i = 0; i != count; i++)
  {
    NSString * identifier, * name;
    CateyesHostApplicationInfo * info = &result[i];

    identifier = [identifiers objectAtIndex:i];
    name = api->SBSCopyLocalizedApplicationNameForDisplayIdentifier (identifier);
    info->_identifier = g_strdup ([identifier UTF8String]);
    info->_name = g_strdup ([name UTF8String]);
    info->_pid = GPOINTER_TO_UINT (g_hash_table_lookup (pid_by_identifier, info->_identifier));
    [name release];

    extract_icons_from_identifier (identifier, &info->_small_icon, &info->_large_icon);
  }

  [identifiers release];

  g_hash_table_unref (pid_by_identifier);

  [pool release];

  return result;
#else
  *result_length = 0;

  return NULL;
#endif
}

CateyesHostProcessInfo *
cateyes_system_enumerate_processes (int * result_length)
{
  NSAutoreleasePool * pool;
  struct kinfo_proc * entries;
  guint count, i;
  CateyesHostProcessInfo * result;

  cateyes_system_init ();

  pool = [[NSAutoreleasePool alloc] init];

  entries = cateyes_system_query_kinfo_procs (&count);

  result = g_new0 (CateyesHostProcessInfo, count);
  *result_length = count;

#ifdef HAVE_IOS
  CateyesSpringboardApi * api = _cateyes_get_springboard_api ();
#endif

  for (i = 0; i != count; i++)
  {
    struct kinfo_proc * e = &entries[i];
    CateyesHostProcessInfo * info = &result[i];

    info->_pid = e->kp_proc.p_pid;

#ifdef HAVE_IOS
    NSString * identifier = api->SBSCopyDisplayIdentifierForProcessID (info->_pid);
    if (identifier != nil)
    {
      NSString * app_name;

      app_name = api->SBSCopyLocalizedApplicationNameForDisplayIdentifier (identifier);
      info->_name = g_strdup ([app_name UTF8String]);
      [app_name release];

      extract_icons_from_identifier (identifier, &info->_small_icon, &info->_large_icon);

      [identifier release];
    }
    else
#endif
    {
#ifdef HAVE_MACOS
      NSRunningApplication * app = [NSRunningApplication runningApplicationWithProcessIdentifier:info->_pid];
      if (app.icon != nil)
      {
        info->_name = g_strdup ([app.localizedName UTF8String]);

        extract_icons_from_image (app.icon, &info->_small_icon, &info->_large_icon);
      }
      else
#endif
      {
        gchar path[PROC_PIDPATHINFO_MAXSIZE];

        proc_pidpath (info->_pid, path, sizeof (path));
        info->_name = g_path_get_basename (path);

        cateyes_image_data_init (&info->_small_icon, 0, 0, 0, "");
        cateyes_image_data_init (&info->_large_icon, 0, 0, 0, "");
      }
    }
  }

  g_free (entries);

  [pool release];

  return result;
}

static struct kinfo_proc *
cateyes_system_query_kinfo_procs (guint * count)
{
  struct kinfo_proc * entries;
  int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
  size_t size;
  gint err;

  err = sysctl (name, G_N_ELEMENTS (name) - 1, NULL, &size, NULL, 0);
  g_assert_cmpint (err, !=, -1);

  entries = g_malloc0 (size);

  err = sysctl (name, G_N_ELEMENTS (name) - 1, entries, &size, NULL, 0);
  g_assert_cmpint (err, !=, -1);

  *count = size / sizeof (struct kinfo_proc);

  return entries;
}

void
cateyes_system_kill (guint pid)
{
  kill (pid, SIGKILL);
}

gchar *
cateyes_temporary_directory_get_system_tmp (void)
{
  if (geteuid () == 0)
  {
#ifdef HAVE_MACOS
    /* Sandboxed system daemons are likely able to read from this location */
    return g_strdup ("/private/var/root");
#else
    return g_strdup ("/Library/Caches");
#endif
  }
  else
  {
#ifdef HAVE_MACOS
    /* Mac App Store apps are sandboxed but able to read ~/.Trash/ */
    return g_build_filename (g_get_home_dir (), ".Trash", ".cateyes", NULL);
#else
    return g_strdup (g_get_tmp_dir ());
#endif
  }
}

#ifdef HAVE_MACOS

static void
extract_icons_from_image (NSImage * image, CateyesImageData * small_icon, CateyesImageData * large_icon)
{
  _cateyes_image_data_init_from_native_image_scaled_to (small_icon, image, 16, 16);
  _cateyes_image_data_init_from_native_image_scaled_to (large_icon, image, 32, 32);
}

#endif

#ifdef HAVE_IOS

static void
extract_icons_from_identifier (NSString * identifier, CateyesImageData * small_icon, CateyesImageData * large_icon)
{
  CateyesIconPair * pair;

  pair = g_hash_table_lookup (icon_pair_by_identifier, [identifier UTF8String]);
  if (pair == NULL)
  {
    NSData * png_data;
    UIImage * image;

    png_data = _cateyes_get_springboard_api ()->SBSCopyIconImagePNGDataForDisplayIdentifier (identifier);

    pair = g_new (CateyesIconPair, 1);
    image = [UIImage imageWithData:png_data];
    _cateyes_image_data_init_from_native_image_scaled_to (&pair->small_icon, image, 16, 16);
    _cateyes_image_data_init_from_native_image_scaled_to (&pair->large_icon, image, 32, 32);
    g_hash_table_insert (icon_pair_by_identifier, g_strdup ([identifier UTF8String]), pair);

    [png_data release];
  }

  cateyes_image_data_copy (&pair->small_icon, small_icon);
  cateyes_image_data_copy (&pair->large_icon, large_icon);
}

#endif /* HAVE_IOS */

static void
cateyes_icon_pair_free (CateyesIconPair * pair)
{
  cateyes_image_data_destroy (&pair->small_icon);
  cateyes_image_data_destroy (&pair->large_icon);
  g_free (pair);
}
