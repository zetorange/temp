#pragma once

#using <PresentationCore.dll>
#using <WindowsBase.dll>

#include <cateyes-core.h>

using namespace System;
using namespace System::Windows::Media;

namespace Cateyes
{
  public ref class Process
  {
  internal:
    Process (CateyesProcess * handle);
  public:
    ~Process ();
  protected:
    !Process ();

  public:
    property unsigned int Pid { unsigned int get (); }
    property String ^ Name { String ^ get (); }
    property ImageSource ^ SmallIcon { ImageSource ^ get (); }
    property ImageSource ^ LargeIcon { ImageSource ^ get (); }

    virtual String ^ ToString () override;

  private:
    CateyesProcess * handle;

    ImageSource ^ smallIcon;
    ImageSource ^ largeIcon;
  };
}