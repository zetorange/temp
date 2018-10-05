/*
 * Copyright (C) 2013-2017 Ole André Vadla Ravnås <oleavr@nowsecure.com>
 *
 * Licence: wxWindows Library Licence, Version 3.1
 */

#include "gummemorymap.h"

#include "gumprocess-priv.h"

typedef struct _GumUpdateMemoryRangesContext GumUpdateMemoryRangesContext;

struct _GumMemoryMapPrivate
{
  GumPageProtection prot;
  GArray * ranges;
  gsize ranges_min;
  gsize ranges_max;
};

struct _GumUpdateMemoryRangesContext
{
  GArray * ranges;
  gint prev_range_index;
};

static void gum_memory_map_finalize (GObject * object);

static gboolean gum_memory_map_add_range (const GumRangeDetails * details,
    gpointer user_data);

G_DEFINE_TYPE (GumMemoryMap, gum_memory_map, G_TYPE_OBJECT);

static void
gum_memory_map_class_init (GumMemoryMapClass * klass)
{
  GObjectClass * object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GumMemoryMapPrivate));

  object_class->finalize = gum_memory_map_finalize;
}

static void
gum_memory_map_init (GumMemoryMap * self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GUM_TYPE_MEMORY_MAP,
      GumMemoryMapPrivate);

  self->priv->ranges = g_array_new (FALSE, FALSE, sizeof (GumMemoryRange));
}

static void
gum_memory_map_finalize (GObject * object)
{
  GumMemoryMap * self = GUM_MEMORY_MAP (object);

  g_array_free (self->priv->ranges, TRUE);

  G_OBJECT_CLASS (gum_memory_map_parent_class)->finalize (object);
}

GumMemoryMap *
gum_memory_map_new (GumPageProtection prot)
{
  GumMemoryMap * map;

  map = g_object_new (GUM_TYPE_MEMORY_MAP, NULL);
  map->priv->prot = prot;

  gum_memory_map_update (map);

  return map;
}

gboolean
gum_memory_map_contains (GumMemoryMap * self,
                         const GumMemoryRange * range)
{
  GumMemoryMapPrivate * priv = self->priv;
  const GumAddress start = range->base_address;
  const GumAddress end = range->base_address + range->size;
  guint i;

  if (start < priv->ranges_min)
    return FALSE;
  else if (end > priv->ranges_max)
    return FALSE;

  for (i = 0; i < priv->ranges->len; i++)
  {
    GumMemoryRange * r = &g_array_index (priv->ranges, GumMemoryRange, i);
    if (start >= r->base_address && end <= r->base_address + r->size)
      return TRUE;
  }

  return FALSE;
}

void
gum_memory_map_update (GumMemoryMap * self)
{
  GumMemoryMapPrivate * priv = self->priv;
  GumUpdateMemoryRangesContext ctx;

  ctx.ranges = priv->ranges;
  ctx.prev_range_index = -1;

  g_array_set_size (priv->ranges, 0);

  _gum_process_enumerate_ranges (priv->prot, gum_memory_map_add_range, &ctx);

  if (priv->ranges->len > 0)
  {
    GumMemoryRange * first_range, * last_range;

    first_range = &g_array_index (priv->ranges, GumMemoryRange, 0);
    last_range = &g_array_index (priv->ranges, GumMemoryRange,
        priv->ranges->len - 1);

    priv->ranges_min = first_range->base_address;
    priv->ranges_max = last_range->base_address + last_range->size;
  }
  else
  {
    priv->ranges_min = 0;
    priv->ranges_max = 0;
  }
}

static gboolean
gum_memory_map_add_range (const GumRangeDetails * details,
                          gpointer user_data)
{
  GumUpdateMemoryRangesContext * ctx =
      (GumUpdateMemoryRangesContext *) user_data;
  GArray * ranges = ctx->ranges;
  const GumMemoryRange * cur = details->range;
  GumMemoryRange * prev;

  if (ctx->prev_range_index >= 0)
    prev = &g_array_index (ranges, GumMemoryRange, ctx->prev_range_index);
  else
    prev = NULL;

  if (prev != NULL && cur->base_address == prev->base_address + prev->size)
    prev->size += cur->size;
  else
    g_array_append_val (ranges, *cur);

  ctx->prev_range_index = ranges->len - 1;

  return TRUE;
}

