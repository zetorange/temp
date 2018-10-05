/*
 * TODO:
 * - Add authentication or restrict to root.
 * - Fix unload while /dev/cateyes is open.
 */

#include <kern/task.h>
#include <libkern/OSAtomic.h>
#include <libkern/OSMalloc.h>
#include <mach/task.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <miscfs/devfs/devfs.h>

#define CATEYES_DEVICE_NAME "cateyes"
#define CATEYES_TAG_NAME "re.cateyes.Agent"
#define CATEYES_IOBASE 'R'

#define CATEYES_ENABLE_SPAWN_GATING  _IO   (CATEYES_IOBASE, 1)
#define CATEYES_DISABLE_SPAWN_GATING _IO   (CATEYES_IOBASE, 2)
#define CATEYES_RESUME               _IOW  (CATEYES_IOBASE, 3, pid_t)
#define CATEYES_TASK_FOR_PID         _IOWR (CATEYES_IOBASE, 4, mach_port_t)

#define CATEYES_LOCK() lck_mtx_lock (cateyes_lock)
#define CATEYES_UNLOCK() lck_mtx_unlock (cateyes_lock)

#ifndef POLLIN
# define POLLIN 0x0001
#endif
#ifndef POLLRDNORM
# define POLLRDNORM 0x0040
#endif

typedef struct _CateyesSpawnEntry CateyesSpawnEntry;
typedef struct _CateyesSpawnNotification CateyesSpawnNotification;
typedef struct _CateyesPrivateApi CateyesPrivateApi;
typedef struct _CateyesMachODetails CateyesMachODetails;

struct _CateyesSpawnEntry
{
  pid_t pid;
  task_t task;
  char executable_path[MAXPATHLEN];

  STAILQ_ENTRY (_CateyesSpawnEntry) entries;
};

struct _CateyesSpawnNotification
{
  char data[11 + 1 + MAXPATHLEN + 1 + 9 + 1 + 1];
  uint16_t offset;
  uint16_t length;

  STAILQ_ENTRY (_CateyesSpawnNotification) notifications;
};

struct _CateyesPrivateApi
{
  kern_return_t (* task_pidsuspend) (task_t task);
  kern_return_t (* task_pidresume) (task_t task);
  ipc_port_t (* convert_task_to_port) (task_t task);
  boolean_t (* is_corpsetask) (task_t task);
  ipc_space_t (* get_task_ipcspace) (task_t task);
  mach_port_name_t (* ipc_port_copyout_send) (ipc_port_t send_right,
      ipc_space_t space);
  void (* ipc_port_release_send) (ipc_port_t port);

  task_t (* proc_task) (proc_t proc);
  int (* proc_pidpathinfo_internal) (proc_t proc, uint64_t arg, char * buf,
      uint32_t buffer_size, int32_t * retval);

  void (** dtrace_proc_waitfor_exec_ptr) (proc_t proc);
};

struct _CateyesMachODetails
{
  const struct mach_header_64 * header;
  const void * linkedit;
  const struct symtab_command * symtab;
};

kern_return_t cateyes_kernel_agent_start (kmod_info_t * ki, void * d);
kern_return_t cateyes_kernel_agent_stop (kmod_info_t * ki, void * d);

static int cateyes_device_open (dev_t dev, int flags, int devtype,
    struct proc * p);
static int cateyes_device_close (dev_t dev, int flags, int devtype,
    struct proc * p);
static int cateyes_device_read (dev_t dev, struct uio * uio, int ioflag);
static int cateyes_device_ioctl (dev_t dev, u_long cmd, caddr_t data, int fflag,
    struct proc * p);
static int cateyes_device_select (dev_t dev, int which, void * wql,
    struct proc * p);

static void cateyes_on_exec (proc_t proc);

static void cateyes_clear_pending (void);

static void cateyes_clear_notifications (void);
static void cateyes_emit_notification (const CateyesSpawnEntry * entry);

static CateyesSpawnEntry * cateyes_spawn_entry_alloc (void);
static void cateyes_spawn_entry_free (CateyesSpawnEntry * self);
static void cateyes_spawn_entry_resume (CateyesSpawnEntry * self);

static CateyesSpawnNotification * cateyes_spawn_notification_alloc (void);
static void cateyes_spawn_notification_free (CateyesSpawnNotification * self);

static bool cateyes_try_enable_exec_hook (void);
static void cateyes_disable_exec_hook (void);
static void cateyes_enable_spawn_gating (void);
static void cateyes_disable_spawn_gating (void);
static int cateyes_resume (pid_t pid);
static int cateyes_task_for_pid (pid_t pid, mach_port_name_t * port);

static bool cateyes_find_private_api (CateyesPrivateApi * api);
static void cateyes_find_kernel_mach_o_details (CateyesMachODetails * details);
static const struct mach_header_64 * cateyes_find_kernel_header (void);

static struct cdevsw cateyes_device =
{
  .d_open = cateyes_device_open,
  .d_close = cateyes_device_close,
  .d_read = cateyes_device_read,
  .d_write = eno_rdwrt,
  .d_ioctl = cateyes_device_ioctl,
  .d_stop = eno_stop,
  .d_reset = eno_reset,
  .d_ttys = NULL,
  .d_select = cateyes_device_select,
  .d_mmap = eno_mmap,
  .d_strategy = eno_strat,
  .d_type = 0
};

static bool cateyes_is_stopping = false;
static int cateyes_num_operations = 0;
static bool cateyes_is_open = false;
static bool cateyes_is_gating = false;
static bool cateyes_is_nonblocking = false;
static struct selinfo * cateyes_selinfo = NULL;
static void * cateyes_selinfo_storage[128];
static STAILQ_HEAD (, _CateyesSpawnEntry) cateyes_pending =
    STAILQ_HEAD_INITIALIZER (cateyes_pending);
static STAILQ_HEAD (, _CateyesSpawnNotification) cateyes_notifications =
    STAILQ_HEAD_INITIALIZER (cateyes_notifications);
static int cateyes_notifications_length = 0;

static CateyesPrivateApi cateyes_private_api;

static int cateyes_device_major;
static void * cateyes_device_node;

static lck_grp_t * cateyes_lock_group;
static lck_grp_attr_t * cateyes_lock_group_attr;

static lck_mtx_t * cateyes_lock;
static lck_attr_t * cateyes_lock_attr;

static OSMallocTag cateyes_tag;

kern_return_t
cateyes_kernel_agent_start (kmod_info_t * ki,
                          void * d)
{
  dev_t dev;

  if (!cateyes_find_private_api (&cateyes_private_api))
    return KERN_FAILURE;

  cateyes_lock_group_attr = lck_grp_attr_alloc_init ();
  cateyes_lock_group = lck_grp_alloc_init ("cateyes", cateyes_lock_group_attr);

  cateyes_lock_attr = lck_attr_alloc_init ();
  cateyes_lock = lck_mtx_alloc_init (cateyes_lock_group, cateyes_lock_attr);

  cateyes_tag = OSMalloc_Tagalloc (CATEYES_TAG_NAME, OSMT_DEFAULT);

  cateyes_device_major = cdevsw_add (-1, &cateyes_device);
  dev = makedev (cateyes_device_major, 0);
  cateyes_device_node = devfs_make_node (dev, DEVFS_CHAR, UID_ROOT, GID_WHEEL,
      0666, CATEYES_DEVICE_NAME);

  return KERN_SUCCESS;
}

kern_return_t
cateyes_kernel_agent_stop (kmod_info_t * ki,
                         void * d)
{
  CATEYES_LOCK ();
  cateyes_is_stopping = true;
  cateyes_disable_exec_hook ();
  CATEYES_UNLOCK ();

  devfs_remove (cateyes_device_node);
  cdevsw_remove (cateyes_device_major, &cateyes_device);

  cateyes_disable_spawn_gating ();

  CATEYES_LOCK ();

  cateyes_is_open = false;

  while (cateyes_num_operations > 0)
  {
    if (cateyes_selinfo != NULL)
      selwakeup (cateyes_selinfo);
    wakeup_one ((caddr_t) &cateyes_notifications_length);

    CATEYES_UNLOCK ();
    CATEYES_LOCK ();
  }

  cateyes_clear_pending ();
  cateyes_clear_notifications ();

  CATEYES_UNLOCK ();

  OSMalloc_Tagfree (cateyes_tag);

  lck_mtx_destroy (cateyes_lock, cateyes_lock_group);
  lck_attr_free (cateyes_lock_attr);

  lck_grp_free (cateyes_lock_group);
  lck_grp_attr_free (cateyes_lock_group_attr);

  return KERN_SUCCESS;
}

static int
cateyes_device_open (dev_t dev,
                   int flags,
                   int devtype,
                   struct proc * p)
{
  CATEYES_LOCK ();

  if (cateyes_is_open)
    goto busy;

  if (!cateyes_try_enable_exec_hook ())
    goto busy;
  cateyes_is_open = true;
  cateyes_is_gating = false;
  cateyes_is_nonblocking = false;

  CATEYES_UNLOCK ();

  return 0;

busy:
  {
    CATEYES_UNLOCK ();

    return EBUSY;
  }
}

static int
cateyes_device_close (dev_t dev,
                    int flags,
                    int devtype,
                    struct proc * p)
{
  CATEYES_LOCK ();

  cateyes_disable_exec_hook ();
  cateyes_is_open = false;
  cateyes_is_gating = false;
  cateyes_is_nonblocking = false;

  cateyes_clear_pending ();
  cateyes_clear_notifications ();

  CATEYES_UNLOCK ();

  return 0;
}

static int
cateyes_device_read (dev_t dev,
                   struct uio * uio,
                   int ioflag)
{
  int error;
  user_ssize_t space_remaining;

  CATEYES_LOCK ();

  cateyes_num_operations++;

  while (cateyes_notifications_length == 0 && !cateyes_is_stopping)
  {
    if (cateyes_is_nonblocking)
      goto would_block;

    error = msleep (&cateyes_notifications_length, cateyes_lock, PRIBIO | PCATCH,
        "cateyes", 0);
    if (error != 0)
      goto propagate_error;
  }

  if (cateyes_is_stopping)
    goto stopping;

  while ((space_remaining = uio_resid (uio)) > 0)
  {
    CateyesSpawnNotification * notification;
    int n;

    notification = STAILQ_FIRST (&cateyes_notifications);
    if (notification == NULL)
      break;

    n = (int) MIN ((user_ssize_t) (notification->length - notification->offset),
        space_remaining);

    error = uiomove (notification->data + notification->offset, n, uio);
    if (error != 0)
      goto propagate_error;

    notification->offset += n;
    if (notification->offset == notification->length)
    {
      STAILQ_REMOVE_HEAD (&cateyes_notifications, notifications);

      cateyes_spawn_notification_free (notification);
    }

    cateyes_notifications_length -= n;
  }

  cateyes_num_operations--;

  CATEYES_UNLOCK ();

  return 0;

would_block:
  {
    error = EAGAIN;

    goto propagate_error;
  }
stopping:
  {
    error = ENOENT;

    goto propagate_error;
  }
propagate_error:
  {
    cateyes_num_operations--;

    CATEYES_UNLOCK ();

    return error;
  }
}

static int
cateyes_device_ioctl (dev_t dev,
                    u_long cmd,
                    caddr_t data,
                    int fflag,
                    struct proc * p)
{
  int error = 0;

  switch (cmd)
  {
    case FIONBIO:
    {
      CATEYES_LOCK ();
      cateyes_is_nonblocking = !!(*(int *) data);
      CATEYES_UNLOCK ();

      break;
    }
    case FIOASYNC:
    {
      if (*(int *) data)
        error = EINVAL;

      break;
    }
    case FIONREAD:
    {
      CATEYES_LOCK ();
      *(int *) data = cateyes_notifications_length;
      CATEYES_UNLOCK ();

      break;
    }
    case CATEYES_ENABLE_SPAWN_GATING:
    {
      cateyes_enable_spawn_gating ();

      break;
    }
    case CATEYES_DISABLE_SPAWN_GATING:
    {
      cateyes_disable_spawn_gating ();

      break;
    }
    case CATEYES_RESUME:
    {
      pid_t pid = *(pid_t *) data;

      error = cateyes_resume (pid);

      break;
    }
    case CATEYES_TASK_FOR_PID:
    {
      pid_t pid = *(pid_t *) data;
      mach_port_name_t port;

      error = cateyes_task_for_pid (pid, &port);
      if (error == 0)
      {
        *(mach_port_name_t *) data = port;
      }

      break;
    }
    default:
    {
      error = ENOTTY;

      break;
    }
  }

  return error;
}

static int
cateyes_device_select (dev_t dev,
                     int which,
                     void * wql,
                     struct proc * p)
{
  int revents;

  revents = 0;

  if ((which & (POLLIN | POLLRDNORM)) != 0)
  {
    CATEYES_LOCK ();

    cateyes_selinfo = (struct selinfo *) &cateyes_selinfo_storage;

    if (cateyes_notifications_length != 0)
      revents |= which & (POLLIN | POLLRDNORM);
    else
      selrecord (p, cateyes_selinfo, wql);

    CATEYES_UNLOCK ();
  }

  return revents;
}

static void
cateyes_on_exec (proc_t proc)
{
  CateyesSpawnEntry * entry;

  CATEYES_LOCK ();

  if (!cateyes_is_open)
    goto not_open;

  entry = cateyes_spawn_entry_alloc ();

  entry->pid = proc_pid (proc);

  if (cateyes_is_gating)
  {
    entry->task = cateyes_private_api.proc_task (proc);
    task_reference (entry->task);
    cateyes_private_api.task_pidsuspend (entry->task);
  }
  else
  {
    entry->task = NULL;
  }

  entry->executable_path[0] = '\0';
  cateyes_private_api.proc_pidpathinfo_internal (proc, 0, entry->executable_path,
      sizeof (entry->executable_path), NULL);

  cateyes_emit_notification (entry);

  if (cateyes_is_gating)
  {
    STAILQ_INSERT_TAIL (&cateyes_pending, entry, entries);
  }
  else
  {
    cateyes_spawn_entry_free (entry);
  }

  CATEYES_UNLOCK ();

  return;

not_open:
  {
    CATEYES_UNLOCK ();

    return;
  }
}

static void
cateyes_clear_pending (void)
{
  CateyesSpawnEntry * entry;

  while ((entry = STAILQ_FIRST (&cateyes_pending)) != NULL)
  {
    STAILQ_REMOVE_HEAD (&cateyes_pending, entries);

    cateyes_spawn_entry_resume (entry);
    cateyes_spawn_entry_free (entry);
  }
}

static void
cateyes_clear_notifications (void)
{
  CateyesSpawnNotification * notification;

  while ((notification = STAILQ_FIRST (&cateyes_notifications)) != NULL)
  {
    STAILQ_REMOVE_HEAD (&cateyes_notifications, notifications);

    cateyes_spawn_notification_free (notification);
  }
}

static void
cateyes_emit_notification (const CateyesSpawnEntry * entry)
{
  CateyesSpawnNotification * notification;

  notification = cateyes_spawn_notification_alloc ();
  snprintf (notification->data, sizeof (notification->data),
      "%d:%s:%s\n", entry->pid, entry->executable_path,
      (entry->task == NULL) ? "running" : "suspended");
  notification->offset = 0;
  notification->length = (int) strlen (notification->data);

  STAILQ_INSERT_TAIL (&cateyes_notifications, notification, notifications);

  cateyes_notifications_length += notification->length;

  if (cateyes_selinfo != NULL)
    selwakeup (cateyes_selinfo);
  wakeup_one ((caddr_t) &cateyes_notifications_length);
}

static CateyesSpawnEntry *
cateyes_spawn_entry_alloc (void)
{
  return OSMalloc (sizeof (CateyesSpawnEntry), cateyes_tag);
}

static void
cateyes_spawn_entry_free (CateyesSpawnEntry * self)
{
  OSFree (self, sizeof (CateyesSpawnEntry), cateyes_tag);
}

static void
cateyes_spawn_entry_resume (CateyesSpawnEntry * self)
{
  cateyes_private_api.task_pidresume (self->task);
  task_deallocate (self->task);
  self->task = NULL;
}

static CateyesSpawnNotification *
cateyes_spawn_notification_alloc (void)
{
  return OSMalloc (sizeof (CateyesSpawnNotification), cateyes_tag);
}

static void
cateyes_spawn_notification_free (CateyesSpawnNotification * self)
{
  OSFree (self, sizeof (CateyesSpawnNotification), cateyes_tag);
}

static bool
cateyes_try_enable_exec_hook (void)
{
  return OSCompareAndSwapPtr (NULL, cateyes_on_exec,
      cateyes_private_api.dtrace_proc_waitfor_exec_ptr);
}

static void
cateyes_disable_exec_hook (void)
{
  OSCompareAndSwapPtr (cateyes_on_exec, NULL,
      cateyes_private_api.dtrace_proc_waitfor_exec_ptr);
}

static void
cateyes_enable_spawn_gating (void)
{
  CATEYES_LOCK ();

  cateyes_is_gating = true;

  CATEYES_UNLOCK ();
}

static void
cateyes_disable_spawn_gating (void)
{
  CATEYES_LOCK ();

  cateyes_is_gating = false;
  cateyes_clear_pending ();

  CATEYES_UNLOCK ();
}

static int
cateyes_resume (pid_t pid)
{
  int error;
  CateyesSpawnEntry * entry;

  error = ESRCH;

  CATEYES_LOCK ();

  STAILQ_FOREACH (entry, &cateyes_pending, entries)
  {
    if (entry->pid == pid)
    {
      STAILQ_REMOVE (&cateyes_pending, entry, _CateyesSpawnEntry, entries);

      cateyes_spawn_entry_resume (entry);
      cateyes_spawn_entry_free (entry);

      error = 0;
      break;
    }
  }

  CATEYES_UNLOCK ();

  return error;
}

static int
cateyes_task_for_pid (pid_t pid,
                    mach_port_name_t * port)
{
  proc_t proc;
  task_t task;
  void * send_right;

  proc = proc_find (pid);
  if (proc == NULL)
    goto not_found;

  task = cateyes_private_api.proc_task (proc);
  task_reference (task);

  send_right = cateyes_private_api.convert_task_to_port (task);

  if (cateyes_private_api.is_corpsetask (task))
    goto task_dead;

  *port = cateyes_private_api.ipc_port_copyout_send (send_right,
      cateyes_private_api.get_task_ipcspace (current_task ()));

  proc_rele (proc);

  return 0;

not_found:
  {
    return ESRCH;
  }
task_dead:
  {
    cateyes_private_api.ipc_port_release_send (send_right);
    proc_rele (proc);
    return ESRCH;
  }
}

static bool
cateyes_find_private_api (CateyesPrivateApi * api)
{
  CateyesMachODetails details;
  const struct symtab_command * symtab;
  const struct nlist_64 * symbols;
  const char * strings;
  uint32_t sym_index;
  int remaining;

  bzero (api, sizeof (CateyesPrivateApi));

  cateyes_find_kernel_mach_o_details (&details);

  symtab = details.symtab;
  symbols = details.linkedit + symtab->symoff;
  strings = details.linkedit + symtab->stroff;

  remaining = 10;
  for (sym_index = 0; sym_index != symtab->nsyms && remaining > 0; sym_index++)
  {
    const struct nlist_64 * symbol = &symbols[sym_index];
    const char * name = strings + symbol->n_un.n_strx;

#   define CATEYES_TRY_RESOLVE(n) \
    if (strcmp (name, "_" OS_STRINGIFY (n)) == 0) \
    { \
      api->n = (void *) symbol->n_value; \
      remaining--; \
      continue; \
    }

    CATEYES_TRY_RESOLVE (task_pidsuspend)
    CATEYES_TRY_RESOLVE (task_pidresume)
    CATEYES_TRY_RESOLVE (convert_task_to_port)
    CATEYES_TRY_RESOLVE (is_corpsetask)
    CATEYES_TRY_RESOLVE (get_task_ipcspace)
    CATEYES_TRY_RESOLVE (ipc_port_copyout_send)
    CATEYES_TRY_RESOLVE (ipc_port_release_send)

    CATEYES_TRY_RESOLVE (proc_task)
    CATEYES_TRY_RESOLVE (proc_pidpathinfo_internal)

    CATEYES_TRY_RESOLVE (dtrace_proc_waitfor_exec_ptr)
  }

  return remaining == 0;
}

static void
cateyes_find_kernel_mach_o_details (CateyesMachODetails * details)
{
  const struct mach_header_64 * header;
  const void * command;
  uint32_t cmd_index;

  header = cateyes_find_kernel_header ();

  details->header = header;
  details->linkedit = NULL;
  details->symtab = NULL;

  command = header + 1;
  for (cmd_index = 0; cmd_index != header->ncmds; cmd_index++)
  {
    const struct load_command * lc = command;

    switch (lc->cmd)
    {
      case LC_SEGMENT_64:
      {
        const struct segment_command_64 * sc = command;

        if (strcmp (sc->segname, "__LINKEDIT") == 0)
        {
          details->linkedit = (const void *) (sc->vmaddr - sc->fileoff);
        }

        break;
      }
      case LC_SYMTAB:
      {
        details->symtab = command;

        break;
      }
    }

    command += lc->cmdsize;
  }
}

static const struct mach_header_64 *
cateyes_find_kernel_header (void)
{
  const void * cur;

  cur = (const void *) ((size_t) OSMalloc_Tagalloc & ~(size_t) (4096 - 1));
  while (true)
  {
    const struct mach_header_64 * header = cur;

    if (header->magic == MH_MAGIC_64)
      return header;

    cur = cur - 4096;
  }

  return NULL;
}
