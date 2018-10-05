#ifndef __CATEYES_WINDOWS_ICON_HELPERS_H__
#define __CATEYES_WINDOWS_ICON_HELPERS_H__

#include "cateyes-core.h"

#define VC_EXTRALEAN
#include <windows.h>
#undef VC_EXTRALEAN

typedef enum _CateyesIconSize CateyesIconSize;

enum _CateyesIconSize
{
  CATEYES_ICON_SMALL,
  CATEYES_ICON_LARGE
};

CateyesImageData * _cateyes_image_data_from_process_or_file (DWORD pid, WCHAR * filename, CateyesIconSize size);

CateyesImageData * _cateyes_image_data_from_process (DWORD pid, CateyesIconSize size);
CateyesImageData * _cateyes_image_data_from_file (WCHAR * filename, CateyesIconSize size);
CateyesImageData * _cateyes_image_data_from_resource_url (WCHAR * resource_url, CateyesIconSize size);

CateyesImageData * _cateyes_image_data_from_native_icon_handle (HICON icon, CateyesIconSize size);

#endif
