#include "Runtime.hpp"

#include <cateyes-core.h>

namespace Cateyes
{
  volatile int Runtime::refCount = 0;

  void Runtime::Ref ()
  {
    g_atomic_int_inc (&refCount);
    cateyes_init ();
  }

  void Runtime::Unref ()
  {
    if (g_atomic_int_dec_and_test (&refCount))
    {
      cateyes_deinit ();
    }
  }

  class Assembly
  {
  public:
    Assembly ()
    {
      Runtime::Ref ();
    }

    ~Assembly ()
    {
      Runtime::Unref ();
    }
  };
  static Assembly assembly;
}