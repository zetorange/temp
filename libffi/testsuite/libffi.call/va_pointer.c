/* Area:		ffi_call
   Purpose:		Test passing pointers in variable argument lists.
   Limitations:	none.
   PR:			none.
   Originator:	        www.cateyes.re */

/* { dg-do run } */

#include "ffitest.h"
#include <stdarg.h>

typedef void * T;

static T
test_fn (T a, T b, ...)
{
  va_list ap;
  T c;

  va_start (ap, b);
  c = va_arg (ap, T);
  printf ("%p %p %p\n", a, b, c);
  va_end (ap);

  return a + 1;
}

int
main (void)
{
  ffi_cif cif;
  ffi_type* arg_types[3];
  T a, b, c;
  T args[3];
  ffi_arg res;

  arg_types[0] = &ffi_type_pointer;
  arg_types[1] = &ffi_type_pointer;
  arg_types[2] = &ffi_type_pointer;

  CHECK(ffi_prep_cif_var (&cif, FFI_DEFAULT_ABI, 2, 3, &ffi_type_pointer, arg_types) == FFI_OK);

  a = (T)0x11223344;
  b = (T)0x55667788;
  c = (T)0xAABBCCDD;
  args[0] = &a;
  args[1] = &b;
  args[2] = &c;

  ffi_call (&cif, FFI_FN (test_fn), &res, args);
  /* { dg-output "0x11223344 0x55667788 0xAABBCCDD" } */
  printf("res: %p\n", (T)res);
  /* { dg-output "\nres: 0x11223345" } */

  return 0;
}
