#import "springboard.h"

#include <dlfcn.h>

#define CATEYES_ASSIGN_SBS_FUNC(N) \
    api->N = dlsym (api->sbs, G_STRINGIFY (N)); \
    g_assert (api->N != NULL)
#define CATEYES_ASSIGN_SBS_CONSTANT(N) \
    str = dlsym (api->sbs, G_STRINGIFY (N)); \
    g_assert (str != NULL); \
    api->N = *str
#define CATEYES_ASSIGN_FBS_CONSTANT(N) \
    str = dlsym (api->fbs, G_STRINGIFY (N)); \
    g_assert (str != NULL); \
    api->N = *str

static CateyesSpringboardApi * cateyes_springboard_api = NULL;

CateyesSpringboardApi *
_cateyes_get_springboard_api (void)
{
  if (cateyes_springboard_api == NULL)
  {
    CateyesSpringboardApi * api;
    NSString ** str;
    id (* objc_get_class_impl) (const gchar * name);

    api = g_new0 (CateyesSpringboardApi, 1);

    api->sbs = dlopen ("/System/Library/PrivateFrameworks/SpringBoardServices.framework/SpringBoardServices", RTLD_GLOBAL | RTLD_LAZY);
    g_assert (api->sbs != NULL);

    api->fbs = dlopen ("/System/Library/PrivateFrameworks/FrontBoardServices.framework/FrontBoardServices", RTLD_GLOBAL | RTLD_LAZY);

    CATEYES_ASSIGN_SBS_FUNC (SBSCopyFrontmostApplicationDisplayIdentifier);
    CATEYES_ASSIGN_SBS_FUNC (SBSCopyApplicationDisplayIdentifiers);
    CATEYES_ASSIGN_SBS_FUNC (SBSCopyDisplayIdentifierForProcessID);
    CATEYES_ASSIGN_SBS_FUNC (SBSCopyLocalizedApplicationNameForDisplayIdentifier);
    CATEYES_ASSIGN_SBS_FUNC (SBSCopyIconImagePNGDataForDisplayIdentifier);
    CATEYES_ASSIGN_SBS_FUNC (SBSLaunchApplicationWithIdentifierAndLaunchOptions);
    CATEYES_ASSIGN_SBS_FUNC (SBSLaunchApplicationWithIdentifierAndURLAndLaunchOptions);
    CATEYES_ASSIGN_SBS_FUNC (SBSApplicationLaunchingErrorString);

    CATEYES_ASSIGN_SBS_CONSTANT (SBSApplicationLaunchOptionUnlockDeviceKey);

    if (api->fbs != NULL)
    {
      objc_get_class_impl = dlsym (RTLD_DEFAULT, "objc_getClass");
      g_assert (objc_get_class_impl != NULL);

      api->FBSSystemService = objc_get_class_impl ("FBSSystemService");
      g_assert (api->FBSSystemService != nil);

      CATEYES_ASSIGN_FBS_CONSTANT (FBSOpenApplicationOptionKeyUnlockDevice);
      CATEYES_ASSIGN_FBS_CONSTANT (FBSOpenApplicationOptionKeyDebuggingOptions);

      CATEYES_ASSIGN_FBS_CONSTANT (FBSDebugOptionKeyArguments);
      CATEYES_ASSIGN_FBS_CONSTANT (FBSDebugOptionKeyEnvironment);
      CATEYES_ASSIGN_FBS_CONSTANT (FBSDebugOptionKeyStandardOutPath);
      CATEYES_ASSIGN_FBS_CONSTANT (FBSDebugOptionKeyStandardErrorPath);
      CATEYES_ASSIGN_FBS_CONSTANT (FBSDebugOptionKeyDisableASLR);
    }

    cateyes_springboard_api = api;
  }

  return cateyes_springboard_api;
}
