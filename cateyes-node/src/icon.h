#ifndef CATEYESNODE_ICON_H
#define CATEYESNODE_ICON_H

#include "glib_object.h"

#include <cateyes-core.h>
#include <nan.h>

namespace cateyes {

class Icon : public GLibObject {
 public:
  static void Init(v8::Handle<v8::Object> exports, Runtime* runtime);
  static v8::Local<v8::Value> New(gpointer handle, Runtime* runtime);

 private:
  explicit Icon(CateyesIcon* handle, Runtime* runtime);
  ~Icon();

  static NAN_METHOD(New);

  static NAN_PROPERTY_GETTER(GetWidth);
  static NAN_PROPERTY_GETTER(GetHeight);
  static NAN_PROPERTY_GETTER(GetRowstride);
  static NAN_PROPERTY_GETTER(GetPixels);

};

}

#endif
