#ifndef CATEYESNODE_APPLICATION_H
#define CATEYESNODE_APPLICATION_H

#include "glib_object.h"

#include <cateyes-core.h>
#include <nan.h>

namespace cateyes {

class Application : public GLibObject {
 public:
  static void Init(v8::Handle<v8::Object> exports, Runtime* runtime);
  static v8::Local<v8::Object> New(gpointer handle, Runtime* runtime);

 private:
  explicit Application(CateyesApplication* handle, Runtime* runtime);
  ~Application();

  static NAN_METHOD(New);

  static NAN_PROPERTY_GETTER(GetIdentifier);
  static NAN_PROPERTY_GETTER(GetName);
  static NAN_PROPERTY_GETTER(GetPid);
  static NAN_PROPERTY_GETTER(GetSmallIcon);
  static NAN_PROPERTY_GETTER(GetLargeIcon);
};

}

#endif
