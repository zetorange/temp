#include "cateyes-core.h"

#include "../windows/icon-helpers.h"

#include <setupapi.h>
#include <devguid.h>

typedef struct _CateyesMobileDeviceInfo CateyesMobileDeviceInfo;
typedef struct _CateyesImageDeviceInfo CateyesImageDeviceInfo;

typedef struct _CateyesFindMobileDeviceContext CateyesFindMobileDeviceContext;
typedef struct _CateyesFindImageDeviceContext CateyesFindImageDeviceContext;

typedef struct _CateyesDeviceInfo CateyesDeviceInfo;

typedef gboolean (* CateyesEnumerateDeviceFunc) (const CateyesDeviceInfo * device_info, gpointer user_data);

struct _CateyesMobileDeviceInfo
{
  WCHAR * location;
};

struct _CateyesImageDeviceInfo
{
  WCHAR * friendly_name;
  WCHAR * icon_url;
};

struct _CateyesFindMobileDeviceContext
{
  const WCHAR * udid;
  CateyesMobileDeviceInfo * mobile_device;
};

struct _CateyesFindImageDeviceContext
{
  const WCHAR * location;
  CateyesImageDeviceInfo * image_device;
};

struct _CateyesDeviceInfo
{
  WCHAR * device_path;
  WCHAR * instance_id;
  WCHAR * friendly_name;
  WCHAR * location;

  HDEVINFO device_info_set;
  PSP_DEVINFO_DATA device_info_data;
};

static CateyesMobileDeviceInfo * find_mobile_device_by_udid (const WCHAR * udid);
static CateyesImageDeviceInfo * find_image_device_by_location (const WCHAR * location);

static gboolean compare_udid_and_create_mobile_device_info_if_matching (const CateyesDeviceInfo * device_info, gpointer user_data);
static gboolean compare_location_and_create_image_device_info_if_matching (const CateyesDeviceInfo * device_info, gpointer user_data);

CateyesMobileDeviceInfo * cateyes_mobile_device_info_new (WCHAR * location);
void cateyes_mobile_device_info_free (CateyesMobileDeviceInfo * mdev);

CateyesImageDeviceInfo * cateyes_image_device_info_new (WCHAR * friendly_name, WCHAR * icon_url);
void cateyes_image_device_info_free (CateyesImageDeviceInfo * idev);

static void cateyes_foreach_usb_device (const GUID * guid, CateyesEnumerateDeviceFunc func, gpointer user_data);

static WCHAR * cateyes_read_device_registry_string_property (HANDLE info_set, SP_DEVINFO_DATA * info_data, DWORD prop_id);
static WCHAR * cateyes_read_registry_string (HKEY key, WCHAR * value_name);
static WCHAR * cateyes_read_registry_multi_string (HKEY key, WCHAR * value_name);
static gpointer cateyes_read_registry_value (HKEY key, WCHAR * value_name, DWORD expected_type);

static GUID GUID_APPLE_USB = { 0xF0B32BE3, 0x6678, 0x4879, 0x92, 0x30, 0x0E4, 0x38, 0x45, 0xD8, 0x05, 0xEE };

void
_cateyes_fruity_host_session_provider_extract_details_for_device (gint product_id, const char * udid, char ** name, CateyesImageData ** icon, GError ** error)
{
  gboolean result = FALSE;
  WCHAR * udid_utf16 = NULL;
  CateyesMobileDeviceInfo * mdev = NULL;
  CateyesImageDeviceInfo * idev = NULL;
  CateyesImageData * idev_icon = NULL;

  (void) product_id;

  udid_utf16 = (WCHAR *) g_utf8_to_utf16 (udid, -1, NULL, NULL, NULL);

  mdev = find_mobile_device_by_udid (udid_utf16);
  if (mdev == NULL)
    goto beach;

  idev = find_image_device_by_location (mdev->location);
  if (idev != NULL)
  {
    idev_icon = _cateyes_image_data_from_resource_url (idev->icon_url, CATEYES_ICON_SMALL);
  }

  if (idev_icon != NULL)
  {
    *name = g_utf16_to_utf8 ((gunichar2 *) idev->friendly_name, -1, NULL, NULL, NULL);
    *icon = idev_icon;
  }
  else
  {
    /* TODO: grab metadata from iTunes instead of relying on having an image device */
    *name = g_strdup ("iOS Device");
    *icon = NULL;
  }
  result = TRUE;

beach:
  if (!result)
  {
    g_set_error (error,
        CATEYES_ERROR,
        CATEYES_ERROR_NOT_SUPPORTED,
        "Unable to extract details for device by UDID '%s'", udid);
  }

  cateyes_image_device_info_free (idev);
  cateyes_mobile_device_info_free (mdev);
  g_free (udid_utf16);
}

static CateyesMobileDeviceInfo *
find_mobile_device_by_udid (const WCHAR * udid)
{
  CateyesFindMobileDeviceContext ctx;

  ctx.udid = udid;
  ctx.mobile_device = NULL;

  cateyes_foreach_usb_device (&GUID_APPLE_USB, compare_udid_and_create_mobile_device_info_if_matching, &ctx);

  return ctx.mobile_device;
}

static CateyesImageDeviceInfo *
find_image_device_by_location (const WCHAR * location)
{
  CateyesFindImageDeviceContext ctx;

  ctx.location = location;
  ctx.image_device = NULL;

  cateyes_foreach_usb_device (&GUID_DEVCLASS_IMAGE, compare_location_and_create_image_device_info_if_matching, &ctx);

  return ctx.image_device;
}

static gboolean
compare_udid_and_create_mobile_device_info_if_matching (const CateyesDeviceInfo * device_info, gpointer user_data)
{
  CateyesFindMobileDeviceContext * ctx = (CateyesFindMobileDeviceContext *) user_data;
  WCHAR * udid, * location;

  udid = wcsrchr (device_info->instance_id, L'\\');
  if (udid == NULL)
    goto keep_looking;
  udid++;

  if (_wcsicmp (udid, ctx->udid) != 0)
    goto keep_looking;

  location = (WCHAR *) g_memdup (device_info->location, ((guint) wcslen (device_info->location) + 1) * sizeof (WCHAR));
  ctx->mobile_device = cateyes_mobile_device_info_new (location);

  return FALSE;

keep_looking:
  return TRUE;
}

static gboolean
compare_location_and_create_image_device_info_if_matching (const CateyesDeviceInfo * device_info, gpointer user_data)
{
  CateyesFindImageDeviceContext * ctx = (CateyesFindImageDeviceContext *) user_data;
  HKEY devkey = (HKEY) INVALID_HANDLE_VALUE;
  WCHAR * friendly_name = NULL;
  WCHAR * icon_url = NULL;

  if (_wcsicmp (device_info->location, ctx->location) != 0)
    goto keep_looking;

  devkey = SetupDiOpenDevRegKey (device_info->device_info_set, device_info->device_info_data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
  if (devkey == INVALID_HANDLE_VALUE)
    goto keep_looking;

  friendly_name = cateyes_read_registry_string (devkey, L"FriendlyName");
  if (friendly_name == NULL)
    goto keep_looking;

  icon_url = cateyes_read_registry_multi_string (devkey, L"Icons");
  if (icon_url == NULL)
    goto keep_looking;

  ctx->image_device = cateyes_image_device_info_new (friendly_name, icon_url);

  RegCloseKey (devkey);
  return FALSE;

keep_looking:
  g_free (icon_url);
  g_free (friendly_name);
  if (devkey != INVALID_HANDLE_VALUE)
    RegCloseKey (devkey);
  return TRUE;
}

CateyesMobileDeviceInfo *
cateyes_mobile_device_info_new (WCHAR * location)
{
  CateyesMobileDeviceInfo * mdev;

  mdev = g_new (CateyesMobileDeviceInfo, 1);
  mdev->location = location;

  return mdev;
}

void
cateyes_mobile_device_info_free (CateyesMobileDeviceInfo * mdev)
{
  if (mdev == NULL)
    return;

  g_free (mdev->location);
  g_free (mdev);
}

CateyesImageDeviceInfo *
cateyes_image_device_info_new (WCHAR * friendly_name, WCHAR * icon_url)
{
  CateyesImageDeviceInfo * idev;

  idev = g_new (CateyesImageDeviceInfo, 1);
  idev->friendly_name = friendly_name;
  idev->icon_url = icon_url;

  return idev;
}

void
cateyes_image_device_info_free (CateyesImageDeviceInfo * idev)
{
  if (idev == NULL)
    return;

  g_free (idev->icon_url);
  g_free (idev->friendly_name);
  g_free (idev);
}

static void
cateyes_foreach_usb_device (const GUID * guid, CateyesEnumerateDeviceFunc func, gpointer user_data)
{
  HANDLE info_set;
  gboolean carry_on = TRUE;
  guint member_index;

  info_set = SetupDiGetClassDevs (guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
  if (info_set == INVALID_HANDLE_VALUE)
    goto beach;

  for (member_index = 0; carry_on; member_index++)
  {
    SP_DEVICE_INTERFACE_DATA iface_data = { 0, };
    SP_DEVINFO_DATA info_data = { 0, };
    DWORD detail_size;
    SP_DEVICE_INTERFACE_DETAIL_DATA_W * detail_data = NULL;
    BOOL success;
    CateyesDeviceInfo device_info = { 0, };
    DWORD instance_id_size;

    iface_data.cbSize = sizeof (iface_data);
    if (!SetupDiEnumDeviceInterfaces (info_set, NULL, guid, member_index, &iface_data))
      break;

    info_data.cbSize = sizeof (info_data);
    success = SetupDiGetDeviceInterfaceDetailW (info_set, &iface_data, NULL, 0, &detail_size, &info_data);
    if (!success && GetLastError () != ERROR_INSUFFICIENT_BUFFER)
      goto skip_device;

    detail_data = (SP_DEVICE_INTERFACE_DETAIL_DATA_W *) g_malloc (detail_size);
    detail_data->cbSize = sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA_W);
    success = SetupDiGetDeviceInterfaceDetailW (info_set, &iface_data, detail_data, detail_size, NULL, &info_data);
    if (!success)
      goto skip_device;

    device_info.device_path = detail_data->DevicePath;

    success = SetupDiGetDeviceInstanceIdW (info_set, &info_data, NULL, 0, &instance_id_size);
    if (!success && GetLastError () != ERROR_INSUFFICIENT_BUFFER)
      goto skip_device;

    device_info.instance_id = (WCHAR *) g_malloc (instance_id_size * sizeof (WCHAR));
    success = SetupDiGetDeviceInstanceIdW (info_set, &info_data, device_info.instance_id, instance_id_size, NULL);
    if (!success)
      goto skip_device;

    device_info.friendly_name = cateyes_read_device_registry_string_property (info_set, &info_data, SPDRP_FRIENDLYNAME);

    device_info.location = cateyes_read_device_registry_string_property (info_set, &info_data, SPDRP_LOCATION_INFORMATION);
    if (device_info.location == NULL)
      goto skip_device;

    device_info.device_info_set = info_set;
    device_info.device_info_data = &info_data;

    carry_on = func (&device_info, user_data);

skip_device:
    g_free (device_info.location);
    g_free (device_info.friendly_name);
    g_free (device_info.instance_id);

    g_free (detail_data);
  }

beach:
  if (info_set != INVALID_HANDLE_VALUE)
    SetupDiDestroyDeviceInfoList (info_set);
}

static WCHAR *
cateyes_read_device_registry_string_property (HANDLE info_set, SP_DEVINFO_DATA * info_data, DWORD prop_id)
{
  gboolean success = FALSE;
  WCHAR * value_buffer = NULL;
  DWORD value_size;
  BOOL ret;

  ret = SetupDiGetDeviceRegistryPropertyW (info_set, info_data, prop_id, NULL, NULL, 0, &value_size);
  if (!ret && GetLastError () != ERROR_INSUFFICIENT_BUFFER)
    goto beach;

  value_buffer = (WCHAR *) g_malloc (value_size);
  if (!SetupDiGetDeviceRegistryPropertyW (info_set, info_data, prop_id, NULL, (PBYTE) value_buffer, value_size, NULL))
    goto beach;

  success = TRUE;

beach:
  if (!success)
  {
    g_free (value_buffer);
    value_buffer = NULL;
  }

  return value_buffer;
}

static WCHAR *
cateyes_read_registry_string (HKEY key, WCHAR * value_name)
{
  return (WCHAR *) cateyes_read_registry_value (key, value_name, REG_SZ);
}

static WCHAR *
cateyes_read_registry_multi_string (HKEY key, WCHAR * value_name)
{
  return (WCHAR *) cateyes_read_registry_value (key, value_name, REG_MULTI_SZ);
}

static gpointer
cateyes_read_registry_value (HKEY key, WCHAR * value_name, DWORD expected_type)
{
  gboolean success = FALSE;
  DWORD type;
  WCHAR * buffer = NULL;
  DWORD base_size = 0, real_size;
  LONG ret;

  ret = RegQueryValueExW (key, value_name, NULL, &type, NULL, &base_size);
  if (ret != ERROR_SUCCESS || type != expected_type)
    goto beach;

  if (type == REG_SZ)
    real_size = base_size + sizeof (WCHAR);
  else if (type == REG_MULTI_SZ)
    real_size = base_size + 2 * sizeof (WCHAR);
  else
    real_size = base_size;
  buffer = (WCHAR *) g_malloc0 (real_size);
  ret = RegQueryValueExW (key, value_name, NULL, &type, (LPBYTE) buffer, &base_size);
  if (ret != ERROR_SUCCESS || type != expected_type)
    goto beach;

  success = TRUE;

beach:
  if (!success)
  {
    g_free (buffer);
    buffer = NULL;
  }

  return buffer;
}
