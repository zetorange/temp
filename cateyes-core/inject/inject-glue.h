#ifndef __CATEYES_INJECT_GLUE_H__
#define __CATEYES_INJECT_GLUE_H__

#include <glib.h>

G_BEGIN_DECLS

G_GNUC_INTERNAL void cateyes_inject_environment_init (void);
G_GNUC_INTERNAL void cateyes_inject_environment_deinit (void);

G_END_DECLS

#endif
