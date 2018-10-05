/* Area:	closure_call
   Purpose:	Test pointer arguments.
   Limitations:	none.
   PR:		none.
   Originator:	www.cateyes.re */

/*
 * To compile:
 * $CC -Wall -pipe -Os $CFLAGS -I../../aarch64-apple-darwin/include -I../../aarch64-apple-darwin cls_pointer_va.c -o cls_pointer_va -L../../aarch64-apple-darwin/.libs -lffi $LDFLAGS
 */

/* { dg-do run } */

#include "ffitest.h"

typedef void * T;

static void cls_ret_T_fn(ffi_cif* cif __UNUSED__, void* resp, void** args,
			 void* userdata __UNUSED__)
 {
   *(T *)(ffi_arg *)resp = *(T *)args[0];

   printf("%p: %p %p %p %p\n", (T)*(ffi_arg *)resp, *(T *)args[0],
	  *(T *)args[1], *(T *)args[2], *(T *)args[3]);
 }

typedef T (*cls_ret_T)(T, T, ...);

int main (void)
{
  ffi_cif cif;
  void *code;
  ffi_closure *pcl = ffi_closure_alloc(sizeof(ffi_closure), &code);
  ffi_type * cl_arg_types[4];
  T res;

  cl_arg_types[0] = &ffi_type_pointer;
  cl_arg_types[1] = &ffi_type_pointer;
  cl_arg_types[2] = &ffi_type_pointer;
  cl_arg_types[3] = &ffi_type_pointer;

  /* Initialize the cif */
  CHECK(ffi_prep_cif_var(&cif, FFI_DEFAULT_ABI, 2, 4,
			 &ffi_type_pointer, cl_arg_types) == FFI_OK);

  CHECK(ffi_prep_closure_loc(pcl, &cif, cls_ret_T_fn, NULL, code)  == FFI_OK);
  res = ((((cls_ret_T)code)((T)0x11223344, (T)0x55667788, (T)0xAABBCCDD,
			    (T)0xEEFF1337)));
  /* { dg-output "0x11223344: 0x11223344 0x55667788 0xAABBCCDD 0xEEFF1337" } */
  printf("res: %p\n", res);
  /* { dg-output "\nres: 0x11223344" } */
  exit(0);
}
