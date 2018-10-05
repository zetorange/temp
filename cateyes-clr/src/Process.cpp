#include "Process.hpp"

#include "Marshal.hpp"
#include "Runtime.hpp"

namespace Cateyes
{
  Process::Process (CateyesProcess * handle)
    : handle (handle),
      smallIcon (nullptr),
      largeIcon (nullptr)
  {
    Runtime::Ref ();
  }

  Process::~Process ()
  {
    if (handle == NULL)
      return;

    if (largeIcon != nullptr)
    {
      delete largeIcon;
      largeIcon = nullptr;
    }
    if (smallIcon != nullptr)
    {
      delete smallIcon;
      smallIcon = nullptr;
    }

    this->!Process ();
  }

  Process::!Process ()
  {
    if (handle != NULL)
    {
      g_object_unref (handle);
      handle = NULL;

      Runtime::Unref ();
    }
  }

  unsigned int
  Process::Pid::get ()
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Process");
    return cateyes_process_get_pid (handle);
  }

  String ^
  Process::Name::get ()
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Process");
    return Marshal::UTF8CStringToClrString (cateyes_process_get_name (handle));
  }

  ImageSource ^
  Process::SmallIcon::get ()
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Process");
    if (smallIcon == nullptr)
      smallIcon = Marshal::CateyesIconToImageSource (cateyes_process_get_small_icon (handle));
    return smallIcon;
  }

  ImageSource ^
  Process::LargeIcon::get ()
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Process");
    if (largeIcon == nullptr)
      largeIcon = Marshal::CateyesIconToImageSource (cateyes_process_get_large_icon (handle));
    return largeIcon;
  }

  String ^
  Process::ToString ()
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Process");
    return String::Format ("Pid: {0}, Name: \"{1}\"", Pid, Name);
  }
}