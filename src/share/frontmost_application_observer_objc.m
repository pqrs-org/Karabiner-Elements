#import "frontmost_application_observer_objc.h"
#import "weakify.h"
#import <Cocoa/Cocoa.h>

@interface KrbnFrontmostApplicationObserver : NSObject

@property krbn_frontmost_application_observer_callback callback;
@property void* context;

@end

@implementation KrbnFrontmostApplicationObserver

- (instancetype)initWithCallback:(krbn_frontmost_application_observer_callback)callback
                         context:(void*)context {
  self = [super init];

  if (self) {
    _callback = callback;
    _context = context;

    [NSWorkspace.sharedWorkspace.notificationCenter addObserver:self
                                                       selector:@selector(handleNotification:)
                                                           name:NSWorkspaceDidActivateApplicationNotification
                                                         object:nil];
  }

  return self;
}

- (void)dealloc {
  [NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)runCallback:(NSRunningApplication*)runningApplication {
  if (self.callback) {
    @try {
      NSString* bundleIdentifier = runningApplication.bundleIdentifier;
      NSString* path = [[runningApplication.executableURL path] stringByStandardizingPath];

      @weakify(self);
      dispatch_async(dispatch_get_main_queue(), ^{
        @strongify(self);
        if (!self) return;

        @try {
          const char* b = [bundleIdentifier UTF8String];
          const char* p = [path UTF8String];

          self.callback(b ? b : "", p ? p : "", self.context);
        } @catch (NSException* exception) {
          NSLog(@"runCallback error");
        }
      });
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

void krbn_frontmost_application_observer_initialize(krbn_frontmost_application_observer_objc** observer,
                                                    krbn_frontmost_application_observer_callback callback,
                                                    void* context) {
  if (!observer) {
    NSLog(@"krbn_frontmost_application_observer_initialize invalid arguments");
    return;
  }
  if (*observer) {
    // observer is already initialized.
    return;
  }

  KrbnFrontmostApplicationObserver* o = [[KrbnFrontmostApplicationObserver alloc] initWithCallback:callback context:context];
  [o runCallbackWithFrontmostApplication];

  *observer = (__bridge_retained void*)(o);
}

void krbn_frontmost_application_observer_terminate(krbn_frontmost_application_observer_objc** observer) {
  if (observer) {
#ifndef __clang_analyzer__
    KrbnFrontmostApplicationObserver* o = (__bridge_transfer KrbnFrontmostApplicationObserver*)(*observer);
    o = nil;
#endif
  }
}
