#import "front_application_observer_objc.h"
#import <Cocoa/Cocoa.h>

@interface KrbnFrontApplicationObserver : NSObject

@property krbn_front_application_observer_callback callback;

@end

@implementation KrbnFrontApplicationObserver

- (instancetype)initWithCallback:(krbn_front_application_observer_callback)callback {
  self = [super init];

  if (self) {
    _callback = callback;

    [NSWorkspace.sharedWorkspace.notificationCenter addObserver:self
                                                       selector:@selector(handleNotification:)
                                                           name:NSWorkspaceDidActivateApplicationNotification
                                                         object:nil];
  }

  NSLog(@"init");

  return self;
}

- (void)dealloc {
  [NSNotificationCenter.defaultCenter removeObserver:self];
  NSLog(@"dealloc");
}

- (void)runCallback:(NSRunningApplication*)runningApplication {
  if (self.callback) {
    @try {
      NSString* bundleIdentifier = runningApplication.bundleIdentifier;
      NSString* path = [[runningApplication.executableURL path] stringByStandardizingPath];
      self.callback([bundleIdentifier UTF8String], [path UTF8String]);
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

- (void)runCallbackWithFrontmostApplication {
  [self runCallback:NSWorkspace.sharedWorkspace.frontmostApplication];
}

@end

void krbn_front_application_observer_initialize(krbn_front_application_observer_objc** observer,
                                                krbn_front_application_observer_callback callback) {
  if (!observer) {
    NSLog(@"krbn_front_application_observer_initialize invalid arguments");
    return;
  }
  if (*observer) {
    // observer is already initialized.
    return;
  }

  KrbnFrontApplicationObserver* o = [[KrbnFrontApplicationObserver alloc] initWithCallback:callback];
  [o runCallbackWithFrontmostApplication];

  *observer = (__bridge_retained void*)(o);
}

void krbn_front_application_observer_terminate(krbn_front_application_observer_objc** observer) {
  if (observer) {
    KrbnFrontApplicationObserver* o = (__bridge_transfer KrbnFrontApplicationObserver*)(*observer);
    o = nil;
  }
}
