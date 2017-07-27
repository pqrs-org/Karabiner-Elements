#import "AppDelegate.h"
#import "AlertWindowController.h"
#import "ComplexModificationsFileImportWindowController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "PreferencesWindowController.h"
#import "SystemPreferencesManager.h"
#import "libkrbn.h"

@interface AppDelegate ()

@property(weak) IBOutlet ComplexModificationsFileImportWindowController* complexModificationsFileImportWindowController;
@property(weak) IBOutlet AlertWindowController* alertWindowController;
@property(weak) IBOutlet NSWindow* preferencesWindow;
@property(weak) IBOutlet PreferencesWindowController* preferencesWindowController;
@property(weak) IBOutlet SystemPreferencesManager* systemPreferencesManager;

@end

@implementation AppDelegate

- (void)applicationWillFinishLaunching:(NSNotification*)notification {
  [[NSAppleEventManager sharedAppleEventManager] setEventHandler:self
                                                     andSelector:@selector(handleGetURLEvent:withReplyEvent:)
                                                   forEventClass:kInternetEventClass
                                                      andEventID:kAEGetURL];
}

- (void)applicationDidFinishLaunching:(NSNotification*)aNotification {
  [[NSApplication sharedApplication] disableRelaunchOnLogin];

  [KarabinerKit setup];
  [KarabinerKit exitIfAnotherProcessIsRunning:"preferences_window.pid"];
  [KarabinerKit observeConsoleUserServerIsDisabledNotification];
  
  [NSThread sleepForTimeInterval:0.3f];
  [self.systemPreferencesManager setup];
  [self.preferencesWindowController setup];

  [self.alertWindowController setup];
}

- (void)handleGetURLEvent:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)replyEvent {
  //  url == @"karabiner://karabiner/assets/complex_modifications/import?url=xxx"
  NSString* url = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
  NSLog(@"handleGetURLEvent %@", url);

  [KarabinerKit endAllAttachedSheets:self.preferencesWindow];

  NSURLComponents* urlComponents = [[NSURLComponents alloc] initWithString:url];
  if ([urlComponents.path isEqualToString:@"/assets/complex_modifications/import"]) {
    for (NSURLQueryItem* pair in urlComponents.queryItems) {
      if ([@"url" isEqualToString:pair.name]) {
        [self.complexModificationsFileImportWindowController setup:pair.value];
        [self.complexModificationsFileImportWindowController show];
        return;
      }
    }
  }

  NSAlert* alert = [NSAlert new];

  alert.messageText = @"Error";
  alert.informativeText = @"Unknown URL";
  [alert addButtonWithTitle:@"OK"];

  [alert beginSheetModalForWindow:self.preferencesWindow
                completionHandler:^(NSModalResponse returnCode){}];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)theApplication {
  return YES;
}

- (BOOL)applicationShouldHandleReopen:(NSApplication*)theApplication hasVisibleWindows:(BOOL)flag {
  [self.preferencesWindowController show];
  return YES;
}

@end
