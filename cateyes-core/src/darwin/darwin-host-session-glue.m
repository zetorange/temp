#include "cateyes-core.h"

#include "icon-helpers.h"

#include <sys/sysctl.h>

#ifdef HAVE_MACOS

typedef struct _CateyesMacModel CateyesMacModel;

struct _CateyesMacModel
{
  const gchar * name;
  const gchar * icon;
};

static const CateyesMacModel mac_models[] =
{
  { NULL,         "com.apple.led-cinema-display-27" },
  { "MacBookAir", "com.apple.macbookair-11-unibody" },
  { "MacBookPro", "com.apple.macbookpro-13-unibody" },
  { "MacBook",    "com.apple.macbook-unibody" },
  { "iMac",       "com.apple.imac-unibody-21" },
  { "Macmini",    "com.apple.macmini-unibody" },
  { "MacPro",     "com.apple.macpro" }
};

#endif

CateyesImageData *
_cateyes_darwin_host_session_provider_try_extract_icon (void)
{
#ifdef HAVE_MACOS
  size_t size;
  gchar * model_name;
  const CateyesMacModel * model;
  guint i;
  gchar * filename;
  CateyesImageData * icon;

  size = 0;
  sysctlbyname ("hw.model", NULL, &size, NULL, 0);
  model_name = g_malloc (size);
  sysctlbyname ("hw.model", model_name, &size, NULL, 0);

  for (model = NULL, i = 1; i != G_N_ELEMENTS (mac_models) && model == NULL; i++)
  {
    if (g_str_has_prefix (model_name, mac_models[i].name))
      model = &mac_models[i];
  }
  if (model == NULL)
    model = &mac_models[0];

  filename = g_strconcat ("/System/Library/CoreServices/CoreTypes.bundle/Contents/Resources/", model->icon, ".icns", NULL);
  icon = _cateyes_image_data_from_file (filename, 16, 16);
  g_free (filename);

  g_free (model_name);

  return icon;
#else
  return NULL;
#endif
}
