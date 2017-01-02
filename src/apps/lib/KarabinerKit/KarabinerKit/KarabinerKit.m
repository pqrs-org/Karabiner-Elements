#import "KarabinerKit.h"

@implementation KarabinerKit

+ (void)setup {
  // initialize managers
  [KarabinerKitConfigurationManager sharedManager];
  [KarabinerKitDeviceManager sharedManager];
}

@end
