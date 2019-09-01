#import "PreferencesController.h"
#import "PreferencesKeys.h"

@interface PreferencesController ()

@property NSMutableArray* oldSettings;
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
      @"targetSettingIsEnabled1" : @NO,
      @"targetSettingIsEnabled2" : @NO,
      @"targetSettingIsEnabled3" : @NO,
      @"targetSettingIsEnabled4" : @NO,
      @"targetSetting1" : @"notsave.thumbsense",
      @"targetSetting2" : @"notsave.enhanced_copyandpaste",
      @"targetSetting3" : @"notsave.pointing_relative_to_scroll",
      @"targetSetting4" : @"notsave.pointing_relative_to_scroll",
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

+ (BOOL)isSettingEnabled:(NSInteger)fingers {
  return [[NSUserDefaults standardUserDefaults] boolForKey:[NSString stringWithFormat:@"targetSettingIsEnabled%d", (int)(fingers)]];
}

+ (NSString*)getSettingIdentifier:(NSInteger)fingers {
  return [[NSUserDefaults standardUserDefaults] stringForKey:[NSString stringWithFormat:@"targetSetting%d", (int)(fingers)]];
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

- (IBAction)set:(id)sender {
  // ------------------------------------------------------------
  // disable old settings

  for (NSString* identifier in self.oldSettings) {
    NSLog(@"set-variables %@ = 0", identifier);
  }

  [self.oldSettings removeAllObjects];
  for (int i = 1; i <= 4; ++i) {
    if ([PreferencesController isSettingEnabled:i]) {
      [self.oldSettings addObject:[PreferencesController getSettingIdentifier:i]];
    }
  }
}

- (void)windowWillClose:(NSNotification*)notification {
  [self set:nil];
}

@end
