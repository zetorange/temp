/*
 * Copyright (C) 2008-2018 Ole André Vadla Ravnås <oleavr@nowsecure.com>
 *
 * Licence: wxWindows Library Licence, Version 3.1
 */

#include "gumdbghelp.h"

#include "gum-init.h"
#include "gumprocess.h"

struct _GumDbgHelpImplPrivate
{
  HMODULE module;
};

static gpointer do_init (gpointer data);
static void do_deinit (void);

static HMODULE load_dbghelp (void);

static void gum_dbghelp_impl_lock (void);
static void gum_dbghelp_impl_unlock (void);

#define INIT_IMPL_FUNC(func) \
    *((gpointer *) (&impl->##func)) = \
        GSIZE_TO_POINTER (GetProcAddress (mod, G_STRINGIFY (func))); \
    g_assert (impl->##func != NULL)

GumDbgHelpImpl *
gum_dbghelp_impl_try_obtain (void)
{
  static GOnce init_once = G_ONCE_INIT;

  g_once (&init_once, do_init, NULL);

  return init_once.retval;
}

static gpointer
do_init (gpointer data)
{
  HMODULE mod;
  GumDbgHelpImpl * impl;

  mod = load_dbghelp ();
  if (mod == NULL)
    return NULL;

  impl = g_slice_new0 (GumDbgHelpImpl);
  impl->priv = g_slice_new (GumDbgHelpImplPrivate);
  impl->priv->module = mod;

  INIT_IMPL_FUNC (StackWalk64);
  INIT_IMPL_FUNC (SymInitialize);
  INIT_IMPL_FUNC (SymCleanup);
  INIT_IMPL_FUNC (SymEnumSymbols);
  INIT_IMPL_FUNC (SymFromAddr);
  INIT_IMPL_FUNC (SymFunctionTableAccess64);
  INIT_IMPL_FUNC (SymGetLineFromAddr64);
  INIT_IMPL_FUNC (SymGetModuleBase64);
  INIT_IMPL_FUNC (SymGetTypeInfo);

  impl->Lock = gum_dbghelp_impl_lock;
  impl->Unlock = gum_dbghelp_impl_unlock;

  impl->SymInitialize (GetCurrentProcess (), NULL, TRUE);

  _gum_register_destructor (do_deinit);

  return impl;
}

static void
do_deinit (void)
{
  GumDbgHelpImpl * impl;

  impl = gum_dbghelp_impl_try_obtain ();
  g_assert (impl != NULL);

  impl->SymCleanup (GetCurrentProcess ());

  FreeLibrary (impl->priv->module);
  g_slice_free (GumDbgHelpImplPrivate, impl->priv);
  g_slice_free (GumDbgHelpImpl, impl);
}

static HMODULE
load_dbghelp (void)
{
  HMODULE mod;
  BOOL success;
  DWORD length;
  WCHAR path[MAX_PATH + 1] = { 0, };
  WCHAR * filename;

  success = GetModuleHandleExW (
      GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
      GUM_FUNCPTR_TO_POINTER (load_dbghelp),
      &mod);
  g_assert (success);

  length = GetModuleFileNameW (mod, path, MAX_PATH);
  g_assert (length != 0);

  filename = wcsrchr (path, L'\\');
  g_assert (filename != NULL);
  filename++;
  wsprintf (filename, L"dbghelp-%d.dll", (GLIB_SIZEOF_VOID_P == 4) ? 32 : 64);

  return LoadLibraryW (path);
}

static GMutex _gum_dbghelp_mutex;

static void
gum_dbghelp_impl_lock (void)
{
  g_mutex_lock (&_gum_dbghelp_mutex);
}

static void
gum_dbghelp_impl_unlock (void)
{
  g_mutex_unlock (&_gum_dbghelp_mutex);
}