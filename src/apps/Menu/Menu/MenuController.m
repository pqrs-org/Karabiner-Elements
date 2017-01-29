#import "MenuController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "libkrbn.h"

@interface MenuController ()

@property(weak) IBOutlet NSMenu* menu;
@property NSStatusItem* statusItem;

@end

@implementation MenuController

- (void)setup {
  KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];
  if (!configurationManager.coreConfigurationModel.globalConfiguration.showInMenuBar) {
    [NSApp terminate:nil];
  }

  NSImage* image = [NSImage imageNamed:@"MenuIcon"];
  [image setTemplate:YES];

  self.statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];

  self.statusItem.button.image = image;
  self.statusItem.button.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
  self.statusItem.button.imagePosition = NSImageLeft;
  self.statusItem.menu = self.menu;

  [self setStatusItemTitle];

  [[NSNotificationCenter defaultCenter] addObserverForName:kKarabinerKitConfigurationIsLoaded
                                                    object:nil
                                                     queue:[NSOperationQueue mainQueue]
                                                usingBlock:^(NSNotification* note) {
                                                  if (!configurationManager.coreConfigurationModel.globalConfiguration.showInMenuBar) {
                                                    [NSApp terminate:nil];
                                                  }

                                                  [self setStatusItemTitle];
                                                }];
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)setStatusItemTitle {
  KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];
  if (configurationManager.coreConfigurationModel.globalConfiguration.showProfileNameInMenuBar) {
    NSString* title = configurationManager.coreConfigurationModel.currentProfile.name;
    if (title) {
      self.statusItem.button.title = title;
      return;
    }
  }

  self.statusItem.button.title = @"";
}

- (void)menuNeedsUpdate:(NSMenu*)menu {
  // --------------------
  // clear
  for (;;) {
    NSMenuItem* item = [menu itemAtIndex:0];
    if (item == nil || [[item title] isEqualToString:@"endoflist"]) break;

    [menu removeItemAtIndex:0];
  }

  // --------------------
  // append

  KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];

  [configurationManager.coreConfigurationModel.profiles enumerateObjectsUsingBlock:^(KarabinerKitConfigurationProfile* profile, NSUInteger i, BOOL* stop) {
    NSMenuItem* newItem = [[NSMenuItem alloc] initWithTitle:profile.name action:@selector(profileSelected:) keyEquivalent:@""];

    [newItem setTarget:self];
    [newItem setRepresentedObject:@(i)];

    if (profile.selected) {
      [newItem setState:NSOnState];
    } else {
      [newItem setState:NSOffState];
    }
    [menu insertItem:newItem atIndex:i];
  }];
}

- (void)profileSelected:(id)sender {
  NSNumber* index = [sender representedObject];

  KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];
  [configurationManager.coreConfigurationModel selectProfile:[index unsignedIntegerValue]];
  [configurationManager save];

  [self setStatusItemTitle];
}

- (IBAction)openPreferences:(id)sender {
  libkrbn_launch_preferences();
}

- (IBAction)checkForUpdates:(id)sender {
  libkrbn_check_for_updates_stable_only();
}

- (IBAction)launchEventViewer:(id)sender {
  libkrbn_launch_event_viewer();
}

- (IBAction)quitKarabiner:(id)sender {
  if ([KarabinerKit quitKarabinerWithConfirmation]) {
    [NSApp terminate:nil];
  }
}

@end
