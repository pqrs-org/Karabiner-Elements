#import "MenuController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "libkrbn.h"

@interface MenuController ()

@property(weak) IBOutlet NSMenu* menu;
@property NSStatusItem* statusItem;

@end

@implementation MenuController

- (void)setup {
  if (![KarabinerKitConfigurationManager sharedManager].coreConfigurationModel2.globalConfigurationShowInMenuBar) {
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
                                                  if (![KarabinerKitConfigurationManager sharedManager].coreConfigurationModel2.globalConfigurationShowInMenuBar) {
                                                    [NSApp terminate:nil];
                                                  }

                                                  [self setStatusItemTitle];
                                                }];
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)setStatusItemTitle {
  KarabinerKitCoreConfigurationModel2* coreConfigurationModel2 = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel2;
  if (coreConfigurationModel2.globalConfigurationShowProfileNameInMenuBar) {
    self.statusItem.button.title = coreConfigurationModel2.selectedProfileName;
    return;
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

  KarabinerKitCoreConfigurationModel2* coreConfigurationModel2 = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel2;
  for (NSUInteger i = 0; i < coreConfigurationModel2.profilesCount; ++i) {
    NSMenuItem* newItem = [[NSMenuItem alloc] initWithTitle:[coreConfigurationModel2 profileNameAtIndex:i]
                                                     action:@selector(profileSelected:)
                                              keyEquivalent:@""];

    [newItem setTarget:self];
    [newItem setRepresentedObject:@(i)];

    if ([coreConfigurationModel2 profileSelectedAtIndex:i]) {
      [newItem setState:NSOnState];
    } else {
      [newItem setState:NSOffState];
    }
    [menu insertItem:newItem atIndex:i];
  }
}

- (void)profileSelected:(id)sender {
  NSNumber* index = [sender representedObject];

  KarabinerKitCoreConfigurationModel2* coreConfigurationModel2 = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel2;
  [coreConfigurationModel2 selectProfileAtIndex:[index unsignedIntegerValue]];
  [coreConfigurationModel2 save];

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
  [KarabinerKit quitKarabinerWithConfirmation];
}

@end
