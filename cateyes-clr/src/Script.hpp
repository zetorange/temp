#pragma once

#using <WindowsBase.dll>

#include <cateyes-core.h>
#include <msclr/gcroot.h>

using namespace System;
using System::Windows::Threading::Dispatcher;

namespace Cateyes
{
  ref class ScriptMessageEventArgs;
  public delegate void ScriptMessageHandler (Object ^ sender, ScriptMessageEventArgs ^ e);

  public ref class Script
  {
  internal:
    Script (CateyesScript * handle, Dispatcher ^ dispatcher);
  public:
    ~Script ();
  protected:
    !Script ();

  public:
    event ScriptMessageHandler ^ Message;

    void Load ();
    void Unload ();
    void Post (String ^ message);
    void PostWithData (String ^ message, array<unsigned char> ^ data);

  internal:
    void OnMessage (Object ^ sender, ScriptMessageEventArgs ^ e);

  private:
    CateyesScript * handle;
    msclr::gcroot<Script ^> * selfHandle;

    Dispatcher ^ dispatcher;
    ScriptMessageHandler ^ onMessageHandler;
  };

  public ref class ScriptMessageEventArgs : public EventArgs
  {
  public:
    property String ^ Message { String ^ get () { return message; } };
    property array<unsigned char> ^ Data { array<unsigned char> ^ get () { return data; } };

    ScriptMessageEventArgs (String ^ message, array<unsigned char> ^ data)
    {
      this->message = message;
      this->data = data;
    }

  private:
    String ^ message;
    array<unsigned char> ^ data;
  };
}
