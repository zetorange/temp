#include "cateyes-gadget.h"

#ifndef G_OS_WIN32
# include "cateyes-interfaces.h"
#endif

#ifdef G_OS_WIN32
# include <process.h>
# define VC_EXTRALEAN
# include <windows.h>
# undef VC_EXTRALEAN
#else
# include <signal.h>
# include <unistd.h>
#endif

#ifdef HAVE_DARWIN
# include <CoreFoundation/CoreFoundation.h>
# include <dlfcn.h>
# include <objc/runtime.h>

# define NSDocumentDirectory 9
# define NSUserDomainMask 1

typedef struct _CateyesFoundationApi CateyesFoundationApi;
typedef struct _CateyesCFApi CateyesCFApi;
typedef struct _CateyesObjCApi CateyesObjCApi;

typedef gsize NSSearchPathDirectory;
typedef gsize NSSearchPathDomainMask;

struct _CateyesFoundationApi
{
  gpointer (* NSSearchPathForDirectoriesInDomains) (NSSearchPathDirectory directory, NSSearchPathDomainMask domain_mask, Boolean expand_tilde);

  Class NSAutoreleasePool;
};

struct _CateyesCFApi
{
  const CFAllocatorRef * kCFAllocatorDefault;
  const CFStringRef * kCFRunLoopCommonModes;

  void (* CFRelease) (CFTypeRef cf);

  CFRunLoopRef (* CFRunLoopGetMain) (void);
  void (* CFRunLoopRun) (void);
  void (* CFRunLoopStop) (CFRunLoopRef loop);
  CFRunLoopTimerRef (* CFRunLoopTimerCreate) (CFAllocatorRef allocator, CFAbsoluteTime fire_date, CFTimeInterval interval, CFOptionFlags flags, CFIndex order, CFRunLoopTimerCallBack callout, CFRunLoopTimerContext * context);
  void (* CFRunLoopAddTimer) (CFRunLoopRef loop, CFRunLoopTimerRef timer, CFStringRef mode);
  void (* CFRunLoopTimerInvalidate) (CFRunLoopTimerRef timer);

  CFBundleRef (* CFBundleGetMainBundle) (void);
  CFStringRef (* CFBundleGetIdentifier) (CFBundleRef bundle);

  CFIndex (* CFStringGetLength) (CFStringRef string);
  CFIndex (* CFStringGetMaximumSizeForEncoding) (CFIndex length, CFStringEncoding encoding);
  Boolean (* CFStringGetCString) (CFStringRef string, char * buffer, CFIndex buffer_size, CFStringEncoding encoding);
};

struct _CateyesObjCApi
{
  Class (* objc_getClass) (const char * name);
  SEL (* sel_registerName) (const char * str);

  void (* objc_msgSend_void_void) (gpointer self, SEL op);
  gpointer (* objc_msgSend_pointer_void) (gpointer self, SEL op);
};

static void on_keep_alive_timer_fire (CFRunLoopTimerRef timer, void * info);

static CateyesFoundationApi * cateyes_foundation_api_try_get (void);
static CateyesCFApi * cateyes_cf_api_try_get (void);
static CateyesObjCApi * cateyes_objc_api_try_get (void);
static gboolean cateyes_dylib_range_try_get (const gchar * apple[], GumMemoryRange * range);

#endif

static gpointer run_main_loop (gpointer data);
static gboolean stop_main_loop (gpointer data);

static GThread * main_thread;
static GMainLoop * main_loop;
static GMainContext * main_context;

#ifdef G_OS_WIN32

BOOL WINAPI
DllMain (HINSTANCE instance, DWORD reason, LPVOID reserved)
{
  (void) instance;
  (void) reserved;

  switch (reason)
  {
    case DLL_PROCESS_ATTACH:
      cateyes_gadget_load (NULL);
      break;
    case DLL_PROCESS_DETACH:
      cateyes_gadget_unload ();
      break;
    default:
      break;
  }

  return TRUE;
}

guint
_cateyes_gadget_getpid (void)
{
  return GetCurrentProcessId ();
}

void
_cateyes_gadget_kill (guint pid)
{
  HANDLE process;

  process = OpenProcess (PROCESS_TERMINATE, FALSE, pid);
  if (process == NULL)
    return;

  TerminateProcess (process, 1);

  CloseHandle (process);
}

#else

# ifdef HAVE_DARWIN

__attribute__ ((constructor)) static void
on_load (int argc, const char * argv[], const char * envp[], const char * apple[])
{
  GumMemoryRange cateyes_dylib_range;

  if (cateyes_dylib_range_try_get (apple, &cateyes_dylib_range))
    cateyes_gadget_load (&cateyes_dylib_range);
  else
    cateyes_gadget_load (NULL);
}

# else

__attribute__ ((constructor)) static void
on_load (void)
{
  cateyes_gadget_load (NULL);
}

# endif

__attribute__ ((destructor)) static void
on_unload (void)
{
  cateyes_gadget_unload ();
}

guint
_cateyes_gadget_getpid (void)
{
  return getpid ();
}

void
_cateyes_gadget_kill (guint pid)
{
  kill (pid, SIGKILL);
}

#endif

#ifdef HAVE_DARWIN

static void
on_keep_alive_timer_fire (CFRunLoopTimerRef timer, void * info)
{
}

#endif

void
cateyes_gadget_environment_init (void)
{
  gum_init_embedded ();

  g_thread_set_garbage_handler (cateyes_gadget_on_pending_garbage, NULL);

  gum_script_backend_get_type (); /* Warm up */
  cateyes_error_quark (); /* Initialize early so GDBus will pick it up */

#ifdef HAVE_DARWIN
  /* Ensure any initializers run on the main thread. */
  cateyes_foundation_api_try_get ();
  cateyes_cf_api_try_get ();
  cateyes_objc_api_try_get ();
#endif

#if defined (HAVE_ANDROID) && __ANDROID_API__ < __ANDROID_API_L__
  /*
   * We might be holding the dynamic linker's lock, so force-initialize
   * our bsd_signal() wrapper on this thread.
   */
  bsd_signal (G_MAXINT32, SIG_DFL);
#endif

  main_context = g_main_context_ref (g_main_context_default ());
  main_loop = g_main_loop_new (main_context, FALSE);
  main_thread = g_thread_new ("gadget-main-loop", run_main_loop, NULL);
}

void
cateyes_gadget_environment_deinit (void)
{
  GSource * source;

  g_assert (main_loop != NULL);

  source = g_idle_source_new ();
  g_source_set_priority (source, G_PRIORITY_LOW);
  g_source_set_callback (source, stop_main_loop, NULL, NULL);
  g_source_attach (source, main_context);
  g_source_unref (source);

  g_thread_join (main_thread);
  main_thread = NULL;

  g_main_loop_unref (main_loop);
  main_loop = NULL;
  g_main_context_unref (main_context);
  main_context = NULL;

  gum_deinit_embedded ();
}

gboolean
cateyes_gadget_environment_can_block_at_load_time (void)
{
#ifdef G_OS_WIN32
  return FALSE;
#else
  return TRUE;
#endif
}

gboolean
cateyes_gadget_environment_has_system_loop (void)
{
#ifdef HAVE_DARWIN
  return cateyes_cf_api_try_get () != NULL;
#else
  return FALSE;
#endif
}

void
cateyes_gadget_environment_run_system_loop (void)
{
#ifdef HAVE_DARWIN
  CateyesCFApi * api;
  CFAbsoluteTime distant_future;
  CFRunLoopTimerRef timer;

  api = cateyes_cf_api_try_get ();
  g_assert (api != NULL);

  distant_future = DBL_MAX;
  timer = api->CFRunLoopTimerCreate (*(api->kCFAllocatorDefault), distant_future, 0, 0, 0, on_keep_alive_timer_fire, NULL);
  api->CFRunLoopAddTimer (api->CFRunLoopGetMain (), timer, *(api->kCFRunLoopCommonModes));

  api->CFRunLoopRun ();

  api->CFRunLoopTimerInvalidate (timer);
  api->CFRelease (timer);
#else
  g_assert_not_reached ();
#endif
}

void
cateyes_gadget_environment_stop_system_loop (void)
{
#ifdef HAVE_DARWIN
  CateyesCFApi * api;

  api = cateyes_cf_api_try_get ();
  g_assert (api != NULL);

  api->CFRunLoopStop (api->CFRunLoopGetMain ());
#else
  g_assert_not_reached ();
#endif
}

GMainContext *
cateyes_gadget_environment_get_main_context (void)
{
  return main_context;
}

GumScriptBackend *
cateyes_gadget_environment_obtain_script_backend (CateyesGadgetRuntimeFlavor runtime)
{
  GumScriptBackend * backend = NULL;

#ifdef HAVE_DIET
  backend = gum_script_backend_obtain_duk ();
#else
  if (runtime == CATEYES_GADGET_RUNTIME_FLAVOR_JIT)
    backend = gum_script_backend_obtain_v8 ();
  if (backend == NULL)
    backend = gum_script_backend_obtain_duk ();
#endif

  return backend;
}

gchar *
cateyes_gadget_environment_detect_bundle_id (void)
{
#ifdef HAVE_DARWIN
  CateyesCFApi * api;
  CFBundleRef bundle;
  CFStringRef identifier;
  CFIndex length, size;
  gchar * identifier_utf8;

  api = cateyes_cf_api_try_get ();
  if (api == NULL)
    return NULL;

  bundle = api->CFBundleGetMainBundle ();
  if (bundle == NULL)
    return NULL;

  identifier = api->CFBundleGetIdentifier (bundle);
  if (identifier == NULL)
    return NULL;

  length = api->CFStringGetLength (identifier);
  size = api->CFStringGetMaximumSizeForEncoding (length, kCFStringEncodingUTF8) + 1;

  identifier_utf8 = g_malloc (size);
  if (!api->CFStringGetCString (identifier, identifier_utf8, size, kCFStringEncodingUTF8))
  {
    g_clear_pointer (&identifier_utf8, g_free);
  }

  return identifier_utf8;
#else
  return NULL;
#endif
}

gchar *
cateyes_gadget_environment_detect_documents_dir (void)
{
#ifdef HAVE_IOS
  CateyesFoundationApi * foundation;
  CateyesObjCApi * objc;
  gpointer pool, paths, path_value;
  gchar * path;

  foundation = cateyes_foundation_api_try_get ();
  if (foundation == NULL)
    return NULL;

  objc = cateyes_objc_api_try_get ();
  g_assert (objc != NULL);

  pool = objc->objc_msgSend_pointer_void (foundation->NSAutoreleasePool, objc->sel_registerName ("alloc"));
  pool = objc->objc_msgSend_pointer_void (pool, objc->sel_registerName ("init"));

  paths = foundation->NSSearchPathForDirectoriesInDomains (NSDocumentDirectory, NSUserDomainMask, TRUE);

  path_value = objc->objc_msgSend_pointer_void (paths, objc->sel_registerName ("firstObject"));

  path = g_strdup (objc->objc_msgSend_pointer_void (path_value, objc->sel_registerName ("UTF8String")));

  objc->objc_msgSend_void_void (pool, objc->sel_registerName ("release"));

  return path;
#else
  return NULL;
#endif
}

gboolean
cateyes_gadget_environment_has_objc_class (const gchar * name)
{
#ifdef HAVE_DARWIN
  CateyesObjCApi * api;

  api = cateyes_objc_api_try_get ();
  if (api == NULL)
    return FALSE;

  return api->objc_getClass (name) != NULL;
#else
  return FALSE;
#endif
}

static gpointer
run_main_loop (gpointer data)
{
  (void) data;

  g_main_context_push_thread_default (main_context);
  g_main_loop_run (main_loop);
  g_main_context_pop_thread_default (main_context);

  return NULL;
}

static gboolean
stop_main_loop (gpointer data)
{
  (void) data;

  g_main_loop_quit (main_loop);

  return FALSE;
}

void
cateyes_gadget_log_info (const gchar * message)
{
  g_info ("%s", message);
}

void
cateyes_gadget_log_warning (const gchar * message)
{
  g_warning ("%s", message);
}

#ifdef HAVE_DARWIN

static CateyesFoundationApi *
cateyes_foundation_api_try_get (void)
{
  static gsize api_value = 0;
  CateyesFoundationApi * api;

  if (g_once_init_enter (&api_value))
  {
    const gchar * foundation_path = "/System/Library/Frameworks/Foundation.framework/Foundation";
    void * foundation;

    foundation = dlopen (foundation_path, RTLD_GLOBAL | RTLD_LAZY | RTLD_NOLOAD);
    if (foundation != NULL)
    {
      CateyesObjCApi * objc;

      /*
       * Okay the process has Foundation loaded!
       *
       * Now let's make sure initializers have been run:
       */
      dlclose (foundation);
      foundation = dlopen (foundation_path, RTLD_GLOBAL | RTLD_LAZY);

      api = g_slice_new (CateyesFoundationApi);

#define CATEYES_ASSIGN_FOUNDATION_SYMBOL(n) \
    api->n = dlsym (foundation, G_STRINGIFY (n)); \
    g_assert (api->n != NULL)

      CATEYES_ASSIGN_FOUNDATION_SYMBOL (NSSearchPathForDirectoriesInDomains);

#undef CATEYES_ASSIGN_FOUNDATION_SYMBOL

      objc = cateyes_objc_api_try_get ();
      g_assert (objc != NULL);

      api->NSAutoreleasePool = objc->objc_getClass ("NSAutoreleasePool");
      g_assert (api->NSAutoreleasePool != NULL);

      dlclose (foundation);
    }
    else
    {
      api = NULL;
    }

    g_once_init_leave (&api_value, 1 + GPOINTER_TO_SIZE (api));
  }

  api = GSIZE_TO_POINTER (api_value - 1);

  return api;
}

static CateyesCFApi *
cateyes_cf_api_try_get (void)
{
  static gsize api_value = 0;
  CateyesCFApi * api;

  if (g_once_init_enter (&api_value))
  {
    const gchar * cf_path = "/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation";
    void * cf;

    cf = dlopen (cf_path, RTLD_GLOBAL | RTLD_LAZY | RTLD_NOLOAD);
    if (cf != NULL)
    {
      dlclose (cf);
      cf = dlopen (cf_path, RTLD_GLOBAL | RTLD_LAZY);

      api = g_slice_new (CateyesCFApi);

#define CATEYES_ASSIGN_CF_SYMBOL(n) \
    api->n = dlsym (cf, G_STRINGIFY (n)); \
    g_assert (api->n != NULL)

      CATEYES_ASSIGN_CF_SYMBOL (kCFAllocatorDefault);
      CATEYES_ASSIGN_CF_SYMBOL (kCFRunLoopCommonModes);

      CATEYES_ASSIGN_CF_SYMBOL (CFRelease);

      CATEYES_ASSIGN_CF_SYMBOL (CFRunLoopGetMain);
      CATEYES_ASSIGN_CF_SYMBOL (CFRunLoopRun);
      CATEYES_ASSIGN_CF_SYMBOL (CFRunLoopStop);
      CATEYES_ASSIGN_CF_SYMBOL (CFRunLoopTimerCreate);
      CATEYES_ASSIGN_CF_SYMBOL (CFRunLoopAddTimer);
      CATEYES_ASSIGN_CF_SYMBOL (CFRunLoopTimerInvalidate);

      CATEYES_ASSIGN_CF_SYMBOL (CFBundleGetMainBundle);
      CATEYES_ASSIGN_CF_SYMBOL (CFBundleGetIdentifier);

      CATEYES_ASSIGN_CF_SYMBOL (CFStringGetLength);
      CATEYES_ASSIGN_CF_SYMBOL (CFStringGetMaximumSizeForEncoding);
      CATEYES_ASSIGN_CF_SYMBOL (CFStringGetCString);

#undef CATEYES_ASSIGN_CF_SYMBOL

      dlclose (cf);
    }
    else
    {
      api = NULL;
    }

    g_once_init_leave (&api_value, 1 + GPOINTER_TO_SIZE (api));
  }

  api = GSIZE_TO_POINTER (api_value - 1);

  return api;
}

static CateyesObjCApi *
cateyes_objc_api_try_get (void)
{
  static gsize api_value = 0;
  CateyesObjCApi * api;

  if (g_once_init_enter (&api_value))
  {
    const gchar * objc_path = "/usr/lib/libobjc.A.dylib";
    void * objc;

    objc = dlopen (objc_path, RTLD_GLOBAL | RTLD_LAZY | RTLD_NOLOAD);
    if (objc != NULL)
    {
      gpointer send_impl;

      dlclose (objc);
      objc = dlopen (objc_path, RTLD_GLOBAL | RTLD_LAZY);

      api = g_slice_new (CateyesObjCApi);

#define CATEYES_ASSIGN_OBJC_SYMBOL(n) \
    api->n = dlsym (objc, G_STRINGIFY (n)); \
    g_assert (api->n != NULL)

      CATEYES_ASSIGN_OBJC_SYMBOL (objc_getClass);
      CATEYES_ASSIGN_OBJC_SYMBOL (sel_registerName);

#undef CATEYES_ASSIGN_OBJC_SYMBOL

      send_impl = dlsym (objc, "objc_msgSend");
      g_assert (send_impl != NULL);

      api->objc_msgSend_void_void = send_impl;
      api->objc_msgSend_pointer_void = send_impl;

      dlclose (objc);
    }
    else
    {
      api = NULL;
    }

    g_once_init_leave (&api_value, 1 + GPOINTER_TO_SIZE (api));
  }

  api = GSIZE_TO_POINTER (api_value - 1);

  return api;
}

static gboolean
cateyes_dylib_range_try_get (const gchar * apple[], GumMemoryRange * range)
{
  const gchar * entry;
  guint i = 0;

  while ((entry = apple[i++]) != NULL)
  {
    if (g_str_has_prefix (entry, "cateyes_dylib_range="))
    {
      if (sscanf (entry, "cateyes_dylib_range=0x%" G_GINT64_MODIFIER "x,0x%" G_GSIZE_MODIFIER "x",
          &range->base_address, &range->size) == 2)
        return TRUE;
    }
  }

  return FALSE;
}

#endif