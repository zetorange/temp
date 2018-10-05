/*
 * Copyright (C) 2017-2018 Ole André Vadla Ravnås <oleavr@nowsecure.com>
 *
 * Licence: wxWindows Library Licence, Version 3.1
 */

#ifndef __GUM_DUK_COMPAT_H__
#define __GUM_DUK_COMPAT_H__

#include <glib.h>

G_BEGIN_DECLS

G_GNUC_INTERNAL double gum_duk_log2 (double x);
G_GNUC_INTERNAL double gum_duk_date_get_now (void);
G_GNUC_INTERNAL double gum_duk_date_get_monotonic_time (void);

G_END_DECLS

#endif
