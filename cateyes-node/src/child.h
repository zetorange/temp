#ifndef CATEYESNODE_CHILD_H
#define CATEYESNODE_CHILD_H

#include "glib_object.h"

#include <cateyes-core.h>

namespace cateyes {

class Child : public GLibObject {
 public:
  static void Init(v8::Handle<v8::Object> exports, Runtime* runtime);
  static v8::Local<v8::Object> New(gpointer handle, Runtime* runtime);

 private:
  explicit Child(CateyesChild* handle, Runtime* runtime);
  ~Child();

  static NAN_METHOD(New);

  static NAN_PROPERTY_GETTER(GetPid);
  static NAN_PROPERTY_GETTER(GetParentPid);
  static NAN_PROPERTY_GETTER(GetOrigin);
  static NAN_PROPERTY_GETTER(GetIdentifier);
  static NAN_PROPERTY_GETTER(GetPath);
  static NAN_PROPERTY_GETTER(GetArgv);
  static NAN_PROPERTY_GETTER(GetEnvp);
};

}

#endif
