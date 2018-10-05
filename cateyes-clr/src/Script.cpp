#include "Script.hpp"

#include "Marshal.hpp"
#include "Runtime.hpp"

using System::Windows::Threading::DispatcherPriority;

namespace Cateyes
{
  static void OnScriptMessage (CateyesScript * script, const gchar * message, GBytes * data, gpointer user_data);

  Script::Script (CateyesScript * handle, Dispatcher ^ dispatcher)
    : handle (handle),
      dispatcher (dispatcher)
  {
    Runtime::Ref ();

    selfHandle = new msclr::gcroot<Script ^> (this);
    onMessageHandler = gcnew ScriptMessageHandler (this, &Script::OnMessage);
    g_signal_connect (handle, "message", G_CALLBACK (OnScriptMessage), selfHandle);
  }

  Script::~Script ()
  {
    if (handle == NULL)
      return;

    g_signal_handlers_disconnect_by_func (handle, OnScriptMessage, selfHandle);
    delete selfHandle;
    selfHandle = NULL;

    this->!Script ();
  }

  Script::!Script ()
  {
    if (handle != NULL)
    {
      g_object_unref (handle);
      handle = NULL;

      Runtime::Unref ();
    }
  }

  void
  Script::Load ()
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Script");

    GError * error = NULL;
    cateyes_script_load_sync (handle, &error);
    Marshal::ThrowGErrorIfSet (&error);
  }

  void
  Script::Unload ()
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Script");

    GError * error = NULL;
    cateyes_script_unload_sync (handle, &error);
    Marshal::ThrowGErrorIfSet (&error);
  }

  void
  Script::Post (String ^ message)
  {
    PostWithData (message, nullptr);
  }

  void
  Script::PostWithData (String ^ message, array<unsigned char> ^ data)
  {
    if (handle == NULL)
      throw gcnew ObjectDisposedException ("Script");

    GError * error = NULL;
    gchar * messageUtf8 = Marshal::ClrStringToUTF8CString (message);
    GBytes * dataBytes = Marshal::ClrByteArrayToBytes (data);
    cateyes_script_post_sync (handle, messageUtf8, dataBytes, &error);
    g_bytes_unref (dataBytes);
    g_free (messageUtf8);
    Marshal::ThrowGErrorIfSet (&error);
  }

  void
  Script::OnMessage (Object ^ sender, ScriptMessageEventArgs ^ e)
  {
    if (dispatcher->CheckAccess ())
      Message (sender, e);
    else
      dispatcher->BeginInvoke (DispatcherPriority::Normal, onMessageHandler, sender, e);
  }

  static void
  OnScriptMessage (CateyesScript * script, const gchar * message, GBytes * data, gpointer user_data)
  {
    (void) script;

    msclr::gcroot<Script ^> * wrapper = static_cast<msclr::gcroot<Script ^> *> (user_data);
    ScriptMessageEventArgs ^ e = gcnew ScriptMessageEventArgs (
        Marshal::UTF8CStringToClrString (message),
        Marshal::BytesToClrArray (data));
   (*wrapper)->OnMessage (*wrapper, e);
  }
}
