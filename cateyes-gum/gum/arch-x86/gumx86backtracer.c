/*
 * Copyright (C) 2008-2011 Ole André Vadla Ravnås <ole.andre.ravnas@tillitech.com>
 *
 * Licence: wxWindows Library Licence, Version 3.1
 */

#include "gumx86backtracer.h"

#include "guminterceptor.h"
#include "gummemorymap.h"

struct _GumX86BacktracerPrivate
{
  GumMemoryMap * code;
  GumMemoryMap * writable;
};

static void gum_x86_backtracer_iface_init (gpointer g_iface,
    gpointer iface_data);
static void gum_x86_backtracer_dispose (GObject * object);
static void gum_x86_backtracer_generate (GumBacktracer * backtracer,
    const GumCpuContext * cpu_context,
    GumReturnAddressArray * return_addresses);

G_DEFINE_TYPE_EXTENDED (GumX86Backtracer,
                        gum_x86_backtracer,
                        G_TYPE_OBJECT,
                        0,
                        G_IMPLEMENT_INTERFACE (GUM_TYPE_BACKTRACER,
                                               gum_x86_backtracer_iface_init));

static void
gum_x86_backtracer_class_init (GumX86BacktracerClass * klass)
{
  GObjectClass * object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GumX86BacktracerPrivate));

  object_class->dispose = gum_x86_backtracer_dispose;
}

static void
gum_x86_backtracer_iface_init (gpointer g_iface,
                               gpointer iface_data)
{
  GumBacktracerIface * iface = (GumBacktracerIface *) g_iface;

  (void) iface_data;

  iface->generate = gum_x86_backtracer_generate;
}

static void
gum_x86_backtracer_init (GumX86Backtracer * self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GUM_TYPE_X86_BACKTRACER,
      GumX86BacktracerPrivate);

  self->priv->code = gum_memory_map_new (GUM_PAGE_EXECUTE);
  self->priv->writable = gum_memory_map_new (GUM_PAGE_WRITE);
}

static void
gum_x86_backtracer_dispose (GObject * object)
{
  GumX86Backtracer * self = GUM_X86_BACKTRACER (object);
  GumX86BacktracerPrivate * priv = self->priv;

  if (priv->code != NULL)
  {
    g_object_unref (priv->code);
    priv->code = NULL;
  }

  if (priv->writable != NULL)
  {
    g_object_unref (priv->writable);
    priv->writable = NULL;
  }

  G_OBJECT_CLASS (gum_x86_backtracer_parent_class)->dispose (object);
}

GumBacktracer *
gum_x86_backtracer_new (void)
{
  return g_object_new (GUM_TYPE_X86_BACKTRACER, NULL);
}

#define OPCODE_CALL_NEAR_RELATIVE     0xE8
#define OPCODE_CALL_NEAR_ABS_INDIRECT 0xFF

static void
gum_x86_backtracer_generate (GumBacktracer * backtracer,
                             const GumCpuContext * cpu_context,
                             GumReturnAddressArray * return_addresses)
{
  GumX86Backtracer * self = GUM_X86_BACKTRACER_CAST (backtracer);
  GumX86BacktracerPrivate * priv = self->priv;
  GumInvocationStack * invocation_stack;
  gsize * start_address;
  gsize first_address = 0;
  guint i;
  gsize * p;

  invocation_stack = gum_interceptor_get_current_stack ();

  if (cpu_context != NULL)
    start_address = GSIZE_TO_POINTER (GUM_CPU_CONTEXT_XSP (cpu_context));
  else
    start_address = (gsize *) &backtracer;

  for (i = 0, p = start_address; p < start_address + 2048; p++)
  {
    gboolean valid = FALSE;
    gsize value;
    GumMemoryRange vr;

    if ((GPOINTER_TO_SIZE (p) & (4096 - 1)) == 0)
    {
      GumMemoryRange next_range;
      next_range.base_address = GUM_ADDRESS (p);
      next_range.size = 4096;
      if (!gum_memory_map_contains (priv->writable, &next_range))
        break;
    }

    value = *p;
    vr.base_address = value - 6;
    vr.size = 6;

    if (value != first_address && value > 4096 + 6 &&
        gum_memory_map_contains (priv->code, &vr))
    {
      gsize translated_value;

      translated_value = GPOINTER_TO_SIZE (gum_invocation_stack_translate (
          invocation_stack, GSIZE_TO_POINTER (value)));
      if (translated_value != value)
      {
        value = translated_value;
        valid = TRUE;
      }
      else
      {
        guint8 * code_ptr = GSIZE_TO_POINTER (value);

        if (*(code_ptr - 5) == OPCODE_CALL_NEAR_RELATIVE ||
            *(code_ptr - 6) == OPCODE_CALL_NEAR_ABS_INDIRECT ||
            *(code_ptr - 3) == OPCODE_CALL_NEAR_ABS_INDIRECT ||
            *(code_ptr - 2) == OPCODE_CALL_NEAR_ABS_INDIRECT)
        {
          valid = TRUE;
        }
      }
    }

    if (valid)
    {
      return_addresses->items[i++] = GSIZE_TO_POINTER (value);
      if (i == G_N_ELEMENTS (return_addresses->items))
        break;

      if (first_address == 0)
        first_address = value;
    }
  }

  return_addresses->len = i;
}

