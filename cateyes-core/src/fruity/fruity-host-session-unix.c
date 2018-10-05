#include "cateyes-core.h"

#include "cateyes-interfaces.h"

void
_cateyes_fruity_host_session_provider_extract_details_for_device (gint product_id, const char * udid, char ** name, CateyesImageData ** icon, GError ** error)
{
  CateyesImageData no_icon = { 0, };

  no_icon._pixels = "";

  *name = g_strdup ("iOS Device");
  *icon = cateyes_image_data_dup (&no_icon);
}
