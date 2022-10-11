#import "SharedlibNameUtil.h"

#include "sharedlib_name/sharedlib_name.h"

#include <string>

@implementation SharedlibNameUtil

+(NSString*)version {
  std::string ver = SHAREDLIB_NAME::GetVersion();
  NSString* ret = [NSString stringWithUTF8String:ver.c_str()];
  return ret;
}

@end
