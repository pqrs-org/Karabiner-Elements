#import "PreferencesController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "PreferencesKeys.h"

@interface PreferencesController ()

@property(weak) IBOutlet NSWindow* preferencesWindow;

@end

@implementation PreferencesController

+ (void)initialize {
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    NSDictionary* dict = @{
      kStartAtLogin : @NO,
      kHideIconInDock : @NO,
      kRelaunchAfterWakeUpFromSleep : @YES,
      kRelaunchWait : @"3",
      kIgnoredAreaTop : @"0",
      kIgnoredAreaBottom : @"0",
      kIgnoredAreaLeft : @"0",
      kIgnoredAreaRight : @"0",
      kDelayBeforeTurnOff : @"0",
      kDelayBeforeTurnOn : @"0",
    };
    [[NSUserDefaults standardUserDefaults] registerDefaults:dict];
  });
}

+ (NSRect)makeTargetArea {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];

  double top = [[defaults stringForKey:kIgnoredAreaTop] doubleValue] / 100;
  double bottom = [[defaults stringForKey:kIgnoredAreaBottom] doubleValue] / 100;
  double left = [[defaults stringForKey:kIgnoredAreaLeft] doubleValue] / 100;
  double right = [[defaults stringForKey:kIgnoredAreaRight] doubleValue] / 100;

  return NSMakeRect(left,
                    bottom,
                    (1.0 - left - right),
                    (1.0 - top - bottom));
}

- (void)show {
  [self.preferencesWindow makeKeyAndOrderFront:nil];
}

- (IBAction)relaunch:(id)sender {
  [KarabinerKit relaunch];
}

@end
