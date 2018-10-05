/*
 * Copyright (C) 2008-2017 Ole André Vadla Ravnås <oleavr@nowsecure.com>
 *
 * Licence: wxWindows Library Licence, Version 3.1
 */

#include "gumdefs.h"

gpointer
gum_cpu_context_get_nth_argument (GumCpuContext * self,
                                  guint n)
{
  gpointer * stack_argument;

#if GLIB_SIZEOF_VOID_P == 4
  stack_argument = (gpointer *) (self->esp + 4);
  return stack_argument[n];
#else
  stack_argument = (gpointer *) (self->rsp + 8);
  switch (n)
  {
# if GUM_NATIVE_ABI_IS_UNIX
    case 0:  return (gpointer) self->rdi;
    case 1:  return (gpointer) self->rsi;
    case 2:  return (gpointer) self->rdx;
    case 3:  return (gpointer) self->rcx;
    case 4:  return (gpointer) self->r8;
    case 5:  return (gpointer) self->r9;
    default: return            stack_argument[n - 6];
# else
    case 0:  return (gpointer) self->rcx;
    case 1:  return (gpointer) self->rdx;
    case 2:  return (gpointer) self->r8;
    case 3:  return (gpointer) self->r9;
    default: return            stack_argument[n];
# endif
  }
#endif
}

void
gum_cpu_context_replace_nth_argument (GumCpuContext * self,
                                      guint n,
                                      gpointer value)
{
  gpointer * stack_argument;

#if GLIB_SIZEOF_VOID_P == 4
  stack_argument = (gpointer *) (self->esp + 4);
  stack_argument[n] = value;
#else
  stack_argument = (gpointer *) (self->rsp + 8);
  switch (n)
  {
# if GUM_NATIVE_ABI_IS_UNIX
    case 0:  self->rdi             = (guint64) value; break;
    case 1:  self->rsi             = (guint64) value; break;
    case 2:  self->rdx             = (guint64) value; break;
    case 3:  self->rcx             = (guint64) value; break;
    case 4:  self->r8              = (guint64) value; break;
    case 5:  self->r9              = (guint64) value; break;
    default: stack_argument[n - 6] =           value; break;
# else
    case 0:  self->rcx             = (guint64) value; break;
    case 1:  self->rdx             = (guint64) value; break;
    case 2:  self->r8              = (guint64) value; break;
    case 3:  self->r9              = (guint64) value; break;
    default: stack_argument[n]     =           value; break;
# endif
  }
#endif
}

gpointer
gum_cpu_context_get_return_value (GumCpuContext * self)
{
#if GLIB_SIZEOF_VOID_P == 4
  return (gpointer) self->eax;
#else
  return (gpointer) self->rax;
#endif
}

void
gum_cpu_context_replace_return_value (GumCpuContext * self,
                                      gpointer value)
{
#if GLIB_SIZEOF_VOID_P == 4
  self->eax = (guint32) value;
#else
  self->rax = (guint64) value;
#endif
}
