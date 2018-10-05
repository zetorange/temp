#include "DeviceManager.hpp"

#include "Device.hpp"
#include "Marshal.hpp"
#include "Runtime.hpp"

using System::Windows::Threading::DispatcherPriority;

namespace Cateyes
{
  static void OnDeviceManagerChanged (CateyesDeviceManager * manager, gpointer user_data);

  DeviceManager::DeviceManager (Dispatcher ^ dispatcher)
    : dispatcher (dispatcher)
  {
    Runtime::Ref ();

    handle = cateyes_device_manager_new ();

    selfHandle = new msclr::gcroot<DeviceManager ^> (this);
    onChangedHandler = gcnew EventHandler (this, &DeviceManager::OnChanged);
    g_signal_connect (handle, "changed", G_CALLBACK (OnDeviceManagerChanged), selfHandle);
  }

  DeviceManager::~DeviceManager ()
  {
    if (handle == NULL)
      return;

    cateyes_device_manager_close_sync (handle);
    g_signal_handlers_disconnect_by_func (handle, OnDeviceManagerChanged, selfHandle);
    delete selfHandle;
    selfHandle = NULL;

    this->!DeviceManager ();
  }

  DeviceManager::!DeviceManager ()
  {
    if (handle != NULL)
    {
      g_object_unref (handle);
      handle = NULL;

      Runtime::Unref ();
    }
  }

  array<Device ^> ^
  DeviceManager::EnumerateDevices ()
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("DeviceManager");

    GError * error = NULL;
    CateyesDeviceList * result = cateyes_device_manager_enumerate_devices_sync (handle, &error);
    Marshal::ThrowGErrorIfSet (&error);

    gint result_length = cateyes_device_list_size (result);
    array<Device ^> ^ devices = gcnew array<Device ^> (result_length);
    for (gint i = 0; i != result_length; i++)
      devices[i] = gcnew Device (cateyes_device_list_get (result, i), dispatcher);

    g_object_unref (result);

    return devices;
  }

  void
  DeviceManager::OnChanged (Object ^ sender, EventArgs ^ e)
  {
    if (dispatcher->CheckAccess ())
      Changed (sender, e);
    else
      dispatcher->BeginInvoke (DispatcherPriority::Normal, onChangedHandler, sender, e);
  }

  static void
  OnDeviceManagerChanged (CateyesDeviceManager * manager, gpointer user_data)
  {
    (void) manager;

    msclr::gcroot<DeviceManager ^> * wrapper = static_cast<msclr::gcroot<DeviceManager ^> *> (user_data);
    (*wrapper)->OnChanged (*wrapper, EventArgs::Empty);
  }
}