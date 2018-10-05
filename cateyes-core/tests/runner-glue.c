#define DEBUG_HEAP_LEAKS 0

#include "cateyes-tests.h"

#ifdef HAVE_ANDROID
# include "cateyes-selinux.h"
#endif

#include <gio/gio.h>
#include <gum/gum.h>

#ifdef G_OS_WIN32
# include <windows.h>
# include <conio.h>
# include <crtdbg.h>
# include <stdio.h>
#endif

void
cateyes_test_environment_init (int * args_length1, char *** args)
{
#if defined (G_OS_WIN32) && DEBUG_HEAP_LEAKS
  int tmp_flag;

  /*_CrtSetBreakAlloc (1337);*/

  _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);
  _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);

  tmp_flag = _CrtSetDbgFlag (_CRTDBG_REPORT_FLAG);

  tmp_flag |= _CRTDBG_ALLOC_MEM_DF;
  tmp_flag |= _CRTDBG_LEAK_CHECK_DF;
  tmp_flag &= ~_CRTDBG_CHECK_CRT_DF;

  _CrtSetDbgFlag (tmp_flag);
#endif

  g_setenv ("G_DEBUG", "fatal-warnings:fatal-criticals", TRUE);
#if DEBUG_HEAP_LEAKS
  g_setenv ("G_SLICE", "always-malloc", TRUE);
#endif
  glib_init ();
  gio_init ();
  g_test_init (args_length1, args, NULL);
  gum_init ();
  cateyes_error_quark (); /* Initialize early so GDBus will pick it up */

#ifdef HAVE_ANDROID
  cateyes_selinux_patch_policy ();
#endif
}

void
cateyes_test_environment_deinit (void)
{
#if DEBUG_HEAP_LEAKS
  gum_shutdown ();
  gio_shutdown ();
  glib_shutdown ();
  gum_deinit ();
  gio_deinit ();
  glib_deinit ();
#endif

#if defined (G_OS_WIN32) && !DEBUG_HEAP_LEAKS
  if (IsDebuggerPresent ())
  {
    printf ("\nPress a key to exit.\n");
    _getch ();
  }
#endif
}

CateyesTestOS
cateyes_test_os (void)
{
#if defined (G_OS_WIN32)
  return CATEYES_TEST_OS_WINDOWS;
#elif defined (HAVE_MACOS)
  return CATEYES_TEST_OS_MACOS;
#elif defined (HAVE_IOS)
  return CATEYES_TEST_OS_IOS;
#elif defined (HAVE_ANDROID)
  return CATEYES_TEST_OS_ANDROID;
#elif defined (HAVE_LINUX)
  return CATEYES_TEST_OS_LINUX;
#elif defined (HAVE_QNX)
  return CATEYES_TEST_OS_QNX;
#endif
}

CateyesTestCPU
cateyes_test_cpu (void)
{
#if defined (HAVE_I386) && GLIB_SIZEOF_VOID_P == 4
  return CATEYES_TEST_CPU_X86_32;
#elif defined (HAVE_I386) && GLIB_SIZEOF_VOID_P == 8
  return CATEYES_TEST_CPU_X86_64;
#elif defined (HAVE_ARM)
  return CATEYES_TEST_CPU_ARM_32;
#elif defined (HAVE_ARM64)
  return CATEYES_TEST_CPU_ARM_64;
#elif defined (HAVE_MIPS)
# if G_BYTE_ORDER == G_LITTLE_ENDIAN
  return CATEYES_TEST_CPU_MIPSEL;
# else
  return CATEYES_TEST_CPU_MIPS;
# endif
#endif
}

CateyesTestLibc
cateyes_test_libc (void)
{
#if defined (G_OS_WIN32)
  return CATEYES_TEST_LIBC_MSVCRT;
#elif defined (HAVE_DARWIN)
  return CATEYES_TEST_LIBC_APPLE;
#elif defined (HAVE_GLIBC)
  return CATEYES_TEST_LIBC_GLIBC;
#elif defined (HAVE_UCLIBC)
  return CATEYES_TEST_LIBC_UCLIBC;
#elif defined (HAVE_ANDROID)
  return CATEYES_TEST_LIBC_BIONIC;
#elif defined (HAVE_QNX)
  return CATEYES_TEST_LIBC_QNX;
#endif
}
