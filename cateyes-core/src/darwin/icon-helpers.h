#ifndef __CATEYES_DARWIN_ICON_HELPERS_H__
#define __CATEYES_DARWIN_ICON_HELPERS_H__

#include "cateyes-core.h"

typedef gpointer CateyesNativeImage;

CateyesImageData * _cateyes_image_data_from_file (const gchar * filename, guint target_width, guint target_height);

void _cateyes_image_data_init_from_native_image_scaled_to (CateyesImageData * data, CateyesNativeImage native_image, guint target_width, guint target_height);

#endif
