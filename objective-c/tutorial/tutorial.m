#import <Foundation/Foundation.h>

@interface SampleClass : NSObject 

- (void)sampleMethod;

@end

@implementation SampleClass

- (void)sampleMethod {
  NSLog(@"Hello world!");
}
- (void)dealloc {
  [super dealloc];
  NSLog(@"Object dealloced");
}

@end

int main() {
  @autoreleasepool {
    NSLog(@"Enter autoreleasepool");
    SampleClass *sampleClass = [[SampleClass alloc] init];
    [sampleClass sampleMethod];
    sampleClass = nil;
    NSLog(@"Exit autoreleasepool");
  }
  return 0;
}
