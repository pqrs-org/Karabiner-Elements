#import "PreferencesManager.h"
#import "PreferencesKeys.h"

@implementation PreferencesManager

+ (void)initialize {
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    NSDictionary* dict = @{
      kForceStayTop : @NO,
      kShowInAllSpaces : @NO,
      kShowHex : @NO,
    };
    [[NSUserDefaults standardUserDefaults] registerDefaults:dict];
  });
}

@end
