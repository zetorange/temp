#include "inject-glue.h"

#ifdef HAVE_ANDROID
# include "cateyes-selinux.h"
#endif

#include <gio/gio.h>
#include <gum/gum.h>

void
cateyes_inject_environment_init (void)
{
  gio_init ();

  gum_init ();

#ifdef HAVE_ANDROID
  cateyes_selinux_patch_policy ();
#endif
}

void
cateyes_inject_environment_deinit (void)
{
  gum_deinit ();

  gio_deinit ();
}
