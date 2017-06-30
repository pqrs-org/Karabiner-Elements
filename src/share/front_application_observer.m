#import "front_application_observer.h"
#import <Cocoa/Cocoa.h>

static krbn_front_application_observer_callback callback_;

@interface KrbnFrontApplicationObserver : NSObject
@end

@implementation KrbnFrontApplicationObserver

- (void)runCallback:(NSRunningApplication*)runningApplication {
  if (callback_) {
    @try {
      NSString* path = [[runningApplication.executableURL path] stringByStandardizingPath];
      callback_([runningApplication.bundleIdentifier UTF8String], [path UTF8String]);
    } @catch (NSException* exception) {
      NSLog(@"runCallback error");
    }
  }
}

- (void)handleNotification:(NSNotification*)notification {
  @try {
    NSRunningApplication* runningApplication = notification.userInfo[NSWorkspaceApplicationKey];
    [self runCallback:runningApplication];
  } @catch (NSException* exception) {
    NSLog(@"notificationHandler error");
  }
}

@end

static KrbnFrontApplicationObserver* observer_;

void krbn_front_application_observer_initialize(krbn_front_application_observer_callback callback) {
  callback_ = callback;

  if (!observer_) {
    @try {
      observer_ = [KrbnFrontApplicationObserver new];

      [observer_ runCallback:[[NSWorkspace sharedWorkspace] frontmostApplication]];

      [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:observer_
                                                             selector:@selector(handleNotification:)
                                                                 name:NSWorkspaceDidActivateApplicationNotification
                                                               object:nil];
    } @catch (NSException* exception) {
      NSLog(@"krbn_front_application_observer_initialize error");
    }
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
