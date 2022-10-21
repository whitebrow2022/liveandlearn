#import "OpencvWrapperUtil.h"

#include "opencv_wrapper/opencv_wrapper.h"

#include <string>

@implementation OpencvWrapperUtil

+(NSString*)version {
  std::string ver = OPENCV_WRAPPER::GetVersion();
  NSString* ret = [NSString stringWithUTF8String:ver.c_str()];
  return ret;
}

@end
