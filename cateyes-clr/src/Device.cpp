#include "Device.hpp"

#include "Marshal.hpp"
#include "Process.hpp"
#include "Runtime.hpp"
#include "Session.hpp"

using System::Windows::Threading::DispatcherPriority;

namespace Cateyes
{
  static void OnDeviceLost (CateyesDevice * device, gpointer user_data);

  Device::Device (CateyesDevice * handle, Dispatcher ^ dispatcher)
    : handle (handle),
      dispatcher (dispatcher),
      icon (nullptr)
  {
    Runtime::Ref ();

    selfHandle = new msclr::gcroot<Device ^> (this);
    onLostHandler = gcnew EventHandler (this, &Device::OnLost);
    g_signal_connect (handle, "lost", G_CALLBACK (OnDeviceLost), selfHandle);
  }

  Device::~Device ()
  {
    if (handle == NULL)
      return;

    delete icon;
    icon = nullptr;
    g_signal_handlers_disconnect_by_func (handle, OnDeviceLost, selfHandle);
    delete selfHandle;
    selfHandle = NULL;

    this->!Device ();
  }

  Device::!Device ()
  {
    if (handle != NULL)
    {
      g_object_unref (handle);
      handle = NULL;

      Runtime::Unref ();
    }
  }

  String ^
  Device::Id::get ()
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Device");
    return Marshal::UTF8CStringToClrString (cateyes_device_get_id (handle));
  }

  String ^
  Device::Name::get ()
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Device");
    return Marshal::UTF8CStringToClrString (cateyes_device_get_name (handle));
  }

  ImageSource ^
  Device::Icon::get ()
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Device");
    if (icon == nullptr)
      icon = Marshal::CateyesIconToImageSource (cateyes_device_get_icon (handle));
    return icon;
  }

  DeviceType
  Device::Type::get ()
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Device");

    switch (cateyes_device_get_dtype (handle))
    {
      case CATEYES_DEVICE_TYPE_LOCAL:
        return DeviceType::Local;
      case CATEYES_DEVICE_TYPE_REMOTE:
        return DeviceType::Remote;
      case CATEYES_DEVICE_TYPE_USB:
        return DeviceType::Usb;
      default:
        g_assert_not_reached ();
    }
  }

  array<Process ^> ^
  Device::EnumerateProcesses ()
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Device");

    GError * error = NULL;
    CateyesProcessList * result = cateyes_device_enumerate_processes_sync (handle, &error);
    Marshal::ThrowGErrorIfSet (&error);

    gint result_length = cateyes_process_list_size (result);
    array<Process ^> ^ processes = gcnew array<Process ^> (result_length);
    for (gint i = 0; i != result_length; i++)
      processes[i] = gcnew Process (cateyes_process_list_get (result, i));

    g_object_unref (result);

    return processes;
  }

  unsigned int
  Device::Spawn (String ^ program, array<String ^> ^ argv, array<String ^> ^ envp, array<String ^> ^ env, String ^ cwd)
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Device");

    gchar * programUtf8 = Marshal::ClrStringToUTF8CString (program);

    CateyesSpawnOptions * options = cateyes_spawn_options_new ();

    if (argv != nullptr)
    {
      gchar ** argvVector = Marshal::ClrStringArrayToUTF8CStringVector (argv);
      cateyes_spawn_options_set_argv (options, argvVector, g_strv_length (argvVector));
      g_strfreev (argvVector);
    }

    if (envp != nullptr)
    {
      gchar ** envpVector = Marshal::ClrStringArrayToUTF8CStringVector (envp);
      cateyes_spawn_options_set_envp (options, envpVector, g_strv_length (envpVector));
      g_strfreev (envpVector);
    }

    if (env != nullptr)
    {
      gchar ** envVector = Marshal::ClrStringArrayToUTF8CStringVector (env);
      cateyes_spawn_options_set_env (options, envVector, g_strv_length (envVector));
      g_strfreev (envVector);
    }

    if (cwd != nullptr)
    {
      gchar * cwdUtf8 = Marshal::ClrStringToUTF8CString (cwd);
      cateyes_spawn_options_set_cwd (options, cwdUtf8);
      g_free (cwdUtf8);
    }

    GError * error = NULL;
    guint pid = cateyes_device_spawn_sync (handle, programUtf8, options, &error);

    g_object_unref (options);
    g_free (programUtf8);

    Marshal::ThrowGErrorIfSet (&error);

    return pid;
  }

  void
  Device::Resume (unsigned int pid)
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Device");

    GError * error = NULL;
    cateyes_device_resume_sync (handle, pid, &error);
    Marshal::ThrowGErrorIfSet (&error);
  }

  Session ^
  Device::Attach (unsigned int pid)
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Device");

    GError * error = NULL;
    CateyesSession * session = cateyes_device_attach_sync (handle, pid, &error);
    Marshal::ThrowGErrorIfSet (&error);

    return gcnew Session (session, dispatcher);
  }

  String ^
  Device::ToString ()
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Device");
    return String::Format ("Id: \"{0}\", Name: \"{1}\", Type: {2}", Id, Name, Type);
  }

  void
  Device::OnLost (Object ^ sender, EventArgs ^ e)
  {
    if (dispatcher->CheckAccess ())
      Lost (sender, e);
    else
      dispatcher->BeginInvoke (DispatcherPriority::Normal, onLostHandler, sender, e);
  }

  static void
  OnDeviceLost (CateyesDevice * device, gpointer user_data)
  {
    (void) device;

    msclr::gcroot<Device ^> * wrapper = static_cast<msclr::gcroot<Device ^> *> (user_data);
    (*wrapper)->OnLost (*wrapper, EventArgs::Empty);
  }
}