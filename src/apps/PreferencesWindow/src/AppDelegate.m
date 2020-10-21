#import "AppDelegate.h"
#import "ComplexModificationsFileImportWindowController.h"
#import "DextAlertWindowController.h"
#import "InputMonitoringAlertWindowController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "Karabiner_Elements-Swift.h"
#import "PreferencesWindowController.h"
#import "SimpleModificationsTableViewController.h"
#import "SystemPreferencesManager.h"
#import "libkrbn/libkrbn.h"

@interface AppDelegate ()

@property(weak) IBOutlet SimpleModificationsTableViewController* simpleModificationsTableViewController;
@property(weak) IBOutlet ComplexModificationsFileImportWindowController* complexModificationsFileImportWindowController;
@property(weak) IBOutlet DextAlertWindowController* dextAlertWindowController;
@property(weak) IBOutlet InputMonitoringAlertWindowController* inputMonitoringAlertWindowController;
@property(weak) IBOutlet NSWindow* preferencesWindow;
@property(weak) IBOutlet PreferencesWindowController* preferencesWindowController;
@property(weak) IBOutlet SystemPreferencesManager* systemPreferencesManager;
@property(weak) IBOutlet AlertWindowsManager* alertWindowsManager;
@property(weak) IBOutlet StateJsonMonitor* stateJsonMonitor;

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
  [KarabinerKit observeConsoleUserServerIsDisabledNotification];

  [self.systemPreferencesManager setup];

  [self.preferencesWindowController setup];

  [self.dextAlertWindowController setup];
  [self.inputMonitoringAlertWindowController setup];
  [self.stateJsonMonitor start];
}

// Note:
// We have to set NSSupportsSuddenTermination `NO` to use `applicationWillTerminate`.
- (void)applicationWillTerminate:(NSNotification*)notification {
  libkrbn_terminate();
}

- (void)handleGetURLEvent:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)replyEvent {
  // - url == @"karabiner://karabiner/assets/complex_modifications/import?url=xxx"
  // - url == @"karabiner://karabiner/simple_modifications/new?json={xxx}"
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
  if ([urlComponents.path isEqualToString:@"/simple_modifications/new"]) {
    [self.simpleModificationsTableViewController openSimpleModificationsTab];

    for (NSURLQueryItem* pair in urlComponents.queryItems) {
      if ([@"json" isEqualToString:pair.name]) {
        [self.simpleModificationsTableViewController addItemFromJson:pair.value];
      }
    }
    return;
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
