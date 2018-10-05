#ifndef __CATEYES_DARWIN_SPRINGBOARD_H__
#define __CATEYES_DARWIN_SPRINGBOARD_H__

#include <glib.h>
#import <UIKit/UIKit.h>

typedef struct _CateyesSpringboardApi CateyesSpringboardApi;
typedef void (^ FBSOpenResultCallback) (NSError * error);
typedef enum _FBProcessKillReason FBProcessKillReason;

enum _FBProcessKillReason
{
  FBProcessKillReasonUnknown,
  FBProcessKillReasonUser,
  FBProcessKillReasonPurge,
  FBProcessKillReasonGracefulPurge,
  FBProcessKillReasonThermal,
  FBProcessKillReasonNone,
  FBProcessKillReasonShutdown,
  FBProcessKillReasonLaunchTest,
  FBProcessKillReasonInsecureDrawing
};

@interface FBSSystemService : NSObject

+ (FBSSystemService *)sharedService;

- (pid_t)pidForApplication:(NSString *)identifier;
- (void)openApplication:(NSString *)identifier
                options:(NSDictionary *)options
             clientPort:(mach_port_t)port
             withResult:(FBSOpenResultCallback)result;
- (void)openURL:(NSURL *)url
    application:(NSString *)identifier
        options:(NSDictionary *)options
     clientPort:(mach_port_t)port
     withResult:(FBSOpenResultCallback)result;
- (void)terminateApplication:(NSString *)identifier
                   forReason:(FBProcessKillReason)reason
                   andReport:(BOOL)report
             withDescription:(NSString *)description;

- (mach_port_t)createClientPort;
- (void)cleanupClientPort:(mach_port_t)port;

@end

struct _CateyesSpringboardApi
{
  void * sbs;
  void * fbs;

  NSString * (* SBSCopyFrontmostApplicationDisplayIdentifier) (void);
  NSArray * (* SBSCopyApplicationDisplayIdentifiers) (BOOL active, BOOL debuggable);
  NSString * (* SBSCopyDisplayIdentifierForProcessID) (UInt32 pid);
  NSString * (* SBSCopyLocalizedApplicationNameForDisplayIdentifier) (NSString * identifier);
  NSData * (* SBSCopyIconImagePNGDataForDisplayIdentifier) (NSString * identifier);
  UInt32 (* SBSLaunchApplicationWithIdentifierAndLaunchOptions) (NSString * identifier, NSDictionary * options, BOOL suspended);
  UInt32 (* SBSLaunchApplicationWithIdentifierAndURLAndLaunchOptions) (NSString * identifier, NSURL * url, NSDictionary * params, NSDictionary * options, BOOL suspended);
  NSString * (* SBSApplicationLaunchingErrorString) (UInt32 error);

  NSString * SBSApplicationLaunchOptionUnlockDeviceKey;

  NSString * FBSOpenApplicationOptionKeyUnlockDevice;
  NSString * FBSOpenApplicationOptionKeyDebuggingOptions;

  NSString * FBSDebugOptionKeyArguments;
  NSString * FBSDebugOptionKeyEnvironment;
  NSString * FBSDebugOptionKeyStandardOutPath;
  NSString * FBSDebugOptionKeyStandardErrorPath;
  NSString * FBSDebugOptionKeyDisableASLR;

  id FBSSystemService;
};

G_GNUC_INTERNAL CateyesSpringboardApi * _cateyes_get_springboard_api (void);

#endif
