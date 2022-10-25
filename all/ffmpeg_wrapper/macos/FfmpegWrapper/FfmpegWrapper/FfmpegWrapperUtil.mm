#import "FfmpegWrapperUtil.h"

#include "ffmpeg_wrapper/ffmpeg_wrapper.h"

#include <string>

@implementation FfmpegWrapperUtil

+(NSString*)version {
  std::string ver = FFMPEG_WRAPPER::GetVersion();
  NSString* ret = [NSString stringWithUTF8String:ver.c_str()];
  return ret;
}

@end
