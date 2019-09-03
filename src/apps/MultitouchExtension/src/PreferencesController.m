#import "PreferencesController.h"
#import "PreferencesKeys.h"

@interface PreferencesController ()

@property NSMutableArray<NSString*>* oldSettings;
@property(weak) IBOutlet NSWindow* preferencesWindow;

@end

@implementation PreferencesController

+ (void)initialize {
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    NSDictionary* dict = @{
      @"startAtLogin" : @NO,
      @"hideIconInDock" : @NO,
      @"relaunchAfterWakeUpFromSleep" : @YES,
      @"relaunchWait" : @"3",
      @"ignoredAreaTop" : @"0",
      @"ignoredAreaBottom" : @"0",
      @"ignoredAreaLeft" : @"0",
      @"ignoredAreaRight" : @"0",
      kDelayBeforeTurnOff : @"0",
      kDelayBeforeTurnOn : @"0",
    };
    [[NSUserDefaults standardUserDefaults] registerDefaults:dict];
  });
}

- (instancetype)init {
  self = [super init];

  if (self) {
    _oldSettings = [NSMutableArray new];
  }

  return self;
}

- (void)show {
  [self.preferencesWindow makeKeyAndOrderFront:nil];
}

+ (NSRect)makeTargetArea {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];

  double top = [[defaults stringForKey:@"ignoredAreaTop"] doubleValue] / 100;
  double bottom = [[defaults stringForKey:@"ignoredAreaBottom"] doubleValue] / 100;
  double left = [[defaults stringForKey:@"ignoredAreaLeft"] doubleValue] / 100;
  double right = [[defaults stringForKey:@"ignoredAreaRight"] doubleValue] / 100;

  return NSMakeRect(left,
                    bottom,
                    (1.0 - left - right),
                    (1.0 - top - bottom));
}

- (void)windowWillClose:(NSNotification*)notification {
  [self set:nil];
}

@end
