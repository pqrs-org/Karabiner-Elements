#import "MenuController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "libkrbn/libkrbn.h"
#import <pqrs/weakify.h>

@interface MenuController ()

@property(weak) IBOutlet NSMenu* menu;
@property NSStatusItem* statusItem;
@property NSImage* menuIcon;
@property KarabinerKitSmartObserverContainer* observers;

@end

@implementation MenuController

- (void)setup {
  {
    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
    if (!coreConfigurationModel.globalConfigurationShowInMenuBar &&
        !coreConfigurationModel.globalConfigurationShowProfileNameInMenuBar) {
      [NSApp terminate:nil];
    }
  }

  self.menuIcon = [NSImage imageNamed:@"MenuIcon"];
  [self.menuIcon setTemplate:YES];

  self.statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];
  self.statusItem.button.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
  self.statusItem.menu = self.menu;

  [self setStatusItemImage];
  [self setStatusItemTitle];

  self.observers = [KarabinerKitSmartObserverContainer new];
  @weakify(self);

  {
    id o = [[NSNotificationCenter defaultCenter] addObserverForName:kKarabinerKitConfigurationIsLoaded
                                                             object:nil
                                                              queue:[NSOperationQueue mainQueue]
                                                         usingBlock:^(NSNotification* note) {
                                                           @strongify(self);
                                                           if (!self) {
                                                             return;
                                                           }

                                                           KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
                                                           if (!coreConfigurationModel.globalConfigurationShowInMenuBar &&
                                                               !coreConfigurationModel.globalConfigurationShowProfileNameInMenuBar) {
                                                             [NSApp terminate:nil];
                                                           }
                                                           [self setStatusItemImage];
                                                           [self setStatusItemTitle];
                                                         }];
    [self.observers addNotificationCenterObserver:o];
  }
}

- (void)setStatusItemImage {
  if ([KarabinerKitConfigurationManager sharedManager].coreConfigurationModel.globalConfigurationShowInMenuBar) {
    self.statusItem.button.imagePosition = NSImageLeft;
    self.statusItem.button.image = self.menuIcon;
  } else {
    self.statusItem.button.image = nil;
  }
}

- (void)setStatusItemTitle {
  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  if (coreConfigurationModel.globalConfigurationShowProfileNameInMenuBar) {
    self.statusItem.button.title = coreConfigurationModel.selectedProfileName;
    self.statusItem.button.imagePosition = NSImageLeft;
    return;
  }

  self.statusItem.button.title = @"";
  self.statusItem.button.imagePosition = NSImageOnly;
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

  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  for (NSUInteger i = 0; i < coreConfigurationModel.profilesCount; ++i) {
    NSMenuItem* newItem = [[NSMenuItem alloc] initWithTitle:[coreConfigurationModel profileNameAtIndex:i]
                                                     action:@selector(profileSelected:)
                                              keyEquivalent:@""];

    [newItem setTarget:self];
    [newItem setRepresentedObject:@(i)];

    if ([coreConfigurationModel profileSelectedAtIndex:i]) {
      [newItem setState:NSOnState];
    } else {
      [newItem setState:NSOffState];
    }
    [menu insertItem:newItem atIndex:i];
  }
}

- (void)profileSelected:(id)sender {
  NSNumber* index = [sender representedObject];

  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  [coreConfigurationModel selectProfileAtIndex:[index unsignedIntegerValue]];
  [coreConfigurationModel save];

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
