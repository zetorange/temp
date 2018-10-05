#ifndef CATEYESNODE_GLIB_CONTEXT_H
#define CATEYESNODE_GLIB_CONTEXT_H

#include <cateyes-core.h>

#include <functional>

namespace cateyes {

class GLibContext {
public:
  GLibContext(GMainContext* main_context);
  ~GLibContext();

  void Schedule(std::function<void ()> f);
  void Perform(std::function<void ()> f);

private:
  static gboolean InvokeCallback(gpointer data);
  static void DestroyCallback(gpointer data);

  GMainContext* main_context_;
  GMutex mutex_;
  GCond cond_;
};

}

#endif
