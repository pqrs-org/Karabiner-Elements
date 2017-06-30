#import "front_application_observer.h"
#import <Cocoa/Cocoa.h>

static krbn_front_application_observer_callback callback_;

@interface KrbnFrontApplicationObserver : NSObject
@end

@implementation KrbnFrontApplicationObserver

- (void)callback:(NSNotification*)notification {
  if (callback_) {
    @try {
      NSRunningApplication* runningApplication = notification.userInfo[NSWorkspaceApplicationKey];
      NSString* path = [[runningApplication.executableURL path] stringByStandardizingPath];
      callback_([runningApplication.bundleIdentifier UTF8String], [path UTF8String]);
    } @catch (NSException* exception) {
    }
  }
}

@end

static KrbnFrontApplicationObserver* observer_;

void krbn_front_application_observer_initialize(krbn_front_application_observer_callback callback) {
  callback_ = callback;

  if (!observer_) {
    observer_ = [KrbnFrontApplicationObserver new];

    [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:observer_
                                                           selector:@selector(callback:)
                                                               name:NSWorkspaceDidActivateApplicationNotification
                                                             object:nil];
  }
}

void krbn_front_application_observer_terminate(void) {
  [[NSNotificationCenter defaultCenter] removeObserver:observer_];

  if (observer_) {
    observer_ = nil;
  }
  if (callback_) {
    callback_ = nil;
  }
}
