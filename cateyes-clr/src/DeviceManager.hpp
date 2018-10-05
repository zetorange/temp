#pragma once

#using <WindowsBase.dll>

#include <cateyes-core.h>
#include <msclr/gcroot.h>

using namespace System;
using System::Windows::Threading::Dispatcher;

namespace Cateyes
{
  ref class Device;

  public ref class DeviceManager
  {
  public:
    DeviceManager (Dispatcher ^ dispatcher);
    ~DeviceManager ();
  protected:
    !DeviceManager ();

  public:
    event EventHandler ^ Changed;

    array<Device ^> ^ EnumerateDevices ();

  internal:
    void OnChanged (Object ^ sender, EventArgs ^ e);

  private:
    CateyesDeviceManager * handle;
    msclr::gcroot<DeviceManager ^> * selfHandle;

    Dispatcher ^ dispatcher;
    EventHandler ^ onChangedHandler;
  };
}