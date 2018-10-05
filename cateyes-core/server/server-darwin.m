#include "server-darwin.h"

#import <Foundation/Foundation.h>

static volatile BOOL cateyes_run_loop_running = NO;

void
_cateyes_server_start_run_loop (void)
{
  NSRunLoop * loop = [NSRunLoop mainRunLoop];

  cateyes_run_loop_running = YES;
  while (cateyes_run_loop_running && [loop runMode:NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]])
    ;
}

void
_cateyes_server_stop_run_loop (void)
{
  cateyes_run_loop_running = NO;
  CFRunLoopStop ([[NSRunLoop mainRunLoop] getCFRunLoop]);
}
