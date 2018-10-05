#pragma once

namespace Cateyes
{
  class Runtime
  {
  public:
    static void Ref ();
    static void Unref ();

  private:
    static volatile int refCount;
  };
}