#ifndef CATEYESNODE_PROCESS_H
#define CATEYESNODE_PROCESS_H

#include "glib_object.h"

#include <cateyes-core.h>

namespace cateyes {

class Process : public GLibObject {
 public:
  static void Init(v8::Handle<v8::Object> exports, Runtime* runtime);
  static v8::Local<v8::Object> New(gpointer handle, Runtime* runtime);

 private:
  explicit Process(CateyesProcess* handle, Runtime* runtime);
  ~Process();

  static NAN_METHOD(New);

  static NAN_PROPERTY_GETTER(GetPid);
  static NAN_PROPERTY_GETTER(GetName);
  static NAN_PROPERTY_GETTER(GetSmallIcon);
  static NAN_PROPERTY_GETTER(GetLargeIcon);
};

}

#endif
