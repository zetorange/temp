#include "cateyes-tests.h"

typedef struct _CateyesMetricCollectorEntry CateyesMetricCollectorEntry;
typedef guint (* CateyesMetricCollector) (CateyesTestProcess * process);

struct _CateyesMetricCollectorEntry
{
  const gchar * name;
  CateyesMetricCollector collect;
};

#ifdef HAVE_WINDOWS

#include <windows.h>
#include <psapi.h>

static guint
cateyes_collect_memory_footprint (CateyesTestProcess * process)
{
  PROCESS_MEMORY_COUNTERS_EX counters;
  BOOL success;

  success = GetProcessMemoryInfo (cateyes_test_process_get_handle (process), (PPROCESS_MEMORY_COUNTERS) &counters,
      sizeof (counters));
  g_assert (success);

  return counters.PrivateUsage;
}

static guint
cateyes_collect_handles (CateyesTestProcess * process)
{
  DWORD count;
  BOOL success;

  success = GetProcessHandleCount (cateyes_test_process_get_handle (process), &count);
  g_assert (success);

  return count;
}

#endif

#ifdef HAVE_DARWIN

#ifdef HAVE_IOS
int proc_pid_rusage (int pid, int flavor, rusage_info_t * buffer);
#else
# include <libproc.h>
#endif
#include <mach/mach.h>

static guint
cateyes_collect_memory_footprint (CateyesTestProcess * process)
{
  struct rusage_info_v2 info;
  int res;

  res = proc_pid_rusage (cateyes_test_process_get_id (process), RUSAGE_INFO_V2, (rusage_info_t *) &info);
  g_assert_cmpint (res, ==, 0);

  return info.ri_phys_footprint;
}

static guint
cateyes_collect_mach_ports (CateyesTestProcess * process)
{
  mach_port_t task;
  kern_return_t kr;
  ipc_info_space_basic_t info;

  kr = task_for_pid (mach_task_self (), cateyes_test_process_get_id (process), &task);
  g_assert_cmpint (kr, ==, KERN_SUCCESS);

  kr = mach_port_space_basic_info (task, &info);
  g_assert_cmpint (kr, ==, KERN_SUCCESS);

  kr = mach_port_deallocate (mach_task_self (), task);
  g_assert_cmpint (kr, ==, KERN_SUCCESS);

  return info.iisb_table_inuse;
}

#endif

#ifdef HAVE_LINUX

#include <gum/gum.h>

static guint
cateyes_collect_memory_footprint (CateyesTestProcess * process)
{
  gchar * path, * stats;
  gboolean success;
  gint num_pages;

  path = g_strdup_printf ("/proc/%u/statm", cateyes_test_process_get_id (process));

  success = g_file_get_contents (path, &stats, NULL, NULL);
  g_assert (success);

  num_pages = atoi (strchr (stats,  ' ') + 1); /* RSS */

  g_free (stats);
  g_free (path);

  return num_pages * gum_query_page_size ();
}

static guint
cateyes_collect_file_descriptors (CateyesTestProcess * process)
{
  gchar * path;
  GDir * dir;
  guint count;

  path = g_strdup_printf ("/proc/%u/fd", cateyes_test_process_get_id (process));

  dir = g_dir_open (path, 0, NULL);
  g_assert (dir != NULL);

  count = 0;
  while (g_dir_read_name (dir) != NULL)
    count++;

  g_dir_close (dir);

  g_free (path);

  return count;
}

#endif

static const CateyesMetricCollectorEntry cateyes_metric_collectors[] =
{
#ifdef HAVE_WINDOWS
  { "memory", cateyes_collect_memory_footprint },
  { "handles", cateyes_collect_handles },
#endif
#ifdef HAVE_DARWIN
  { "memory", cateyes_collect_memory_footprint },
  { "ports", cateyes_collect_mach_ports },
#endif
#ifdef HAVE_LINUX
  { "memory", cateyes_collect_memory_footprint },
  { "files", cateyes_collect_file_descriptors },
#endif
  { NULL, NULL }
};

CateyesTestResourceUsageSnapshot *
cateyes_test_process_snapshot_resource_usage (CateyesTestProcess * self)
{
  CateyesTestResourceUsageSnapshot * snapshot;
  const CateyesMetricCollectorEntry * entry;

  snapshot = cateyes_test_resource_usage_snapshot_new ();

  for (entry = cateyes_metric_collectors; entry->name != NULL; entry++)
  {
    guint value;

    value = entry->collect (self);

    g_hash_table_insert (snapshot->metrics, g_strdup (entry->name), GSIZE_TO_POINTER (value));
  }

  return snapshot;
}
