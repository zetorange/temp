#include "cateyes-selinux.h"

#include "patch.h"

#include <stdlib.h>
#include <sys/system_properties.h>

static guint cateyes_get_system_api_level (void);

void
cateyes_selinux_patch_policy (void)
{
  if (cateyes_get_system_api_level () >= 24)
    cateyes_selinux_apply_policy_patch ();
  else
    legacy_cateyes_selinux_apply_policy_patch ();
}

static guint
cateyes_get_system_api_level (void)
{
  gchar sdk_version[PROP_VALUE_MAX];

  sdk_version[0] = '\0';
  __system_property_get ("ro.build.version.sdk", sdk_version);

  return atoi (sdk_version);
}
