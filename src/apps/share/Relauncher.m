#import "Relauncher.h"
#import <CommonCrypto/CommonDigest.h>

@implementation Relauncher

+ (NSString*)getEnvironmentKey {
  NSString* bundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];
  if (!bundleIdentifier) {
    return @"Relauncher_unknownBundleIdentifier";
  }

  unsigned char digest[CC_MD5_DIGEST_LENGTH];
  {
    const char* data = [bundleIdentifier UTF8String];
    CC_LONG len = (CC_LONG)([bundleIdentifier lengthOfBytesUsingEncoding:NSUTF8StringEncoding]);
    CC_MD5(data, len, digest);
  }

  NSMutableString* result = [NSMutableString new];
  [result appendString:@"Relauncher_"];
  for (size_t i = 0; i < sizeof(digest); ++i) {
    [result appendFormat:@"%02x", digest[i]];
  }

  return result;
}

+ (NSString*)getRelaunchedCountEnvironmentKey {
  return [NSString stringWithFormat:@"%@_RC", [Relauncher getEnvironmentKey]];
}

+ (NSString*)getPreviousProcessVersionEnvironmentKey {
  return [NSString stringWithFormat:@"%@_PPV", [Relauncher getEnvironmentKey]];
}

// ------------------------------------------------------------
+ (void)setRelaunchedCount:(int)newvalue {
  const char* key = [[Relauncher getRelaunchedCountEnvironmentKey] UTF8String];
  setenv(key,
         [[NSString stringWithFormat:@"%d", newvalue] UTF8String],
         1);
}

+ (NSInteger)getRelaunchedCount {
  NSString* key = [Relauncher getRelaunchedCountEnvironmentKey];
  return [[[NSProcessInfo processInfo] environment][key] integerValue];
}

+ (void)resetRelaunchedCount {
  [self setRelaunchedCount:0];
}

// ------------------------------------------------------------
+ (NSString*)currentProcessVersion {
  return [[NSBundle mainBundle] infoDictionary][@"CFBundleVersion"];
}

+ (void)setPreviousProcessVersion {
  const char* key = [[Relauncher getPreviousProcessVersionEnvironmentKey] UTF8String];
  setenv(key,
         [[self currentProcessVersion] UTF8String],
         1);
}

+ (BOOL)isEqualPreviousProcessVersionAndCurrentProcessVersion {
  NSString* key = [Relauncher getPreviousProcessVersionEnvironmentKey];

  NSString* previous = [[NSProcessInfo processInfo] environment][key];
  if (!previous) return NO;

  NSString* current = [self currentProcessVersion];

  return [current isEqualToString:previous];
}

// ------------------------------------------------------------
+ (void)relaunch {
  const int RETRY_COUNT = 5;

  NSInteger count = [self getRelaunchedCount];
  if (count < RETRY_COUNT) {
    NSLog(@"Relaunching (count:%d)", (int)(count));
    [self setRelaunchedCount:(int)(count + 1)];
    [self setPreviousProcessVersion];
    [NSTask launchedTaskWithLaunchPath:[[NSBundle mainBundle] executablePath] arguments:@[]];
  } else {
    NSLog(@"Give up relaunching.");
  }

  [NSApp terminate:self];
}

@end
