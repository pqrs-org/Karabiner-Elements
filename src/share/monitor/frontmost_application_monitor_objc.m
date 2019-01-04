#import "frontmost_application_monitor_objc.h"
#import <Cocoa/Cocoa.h>
#import <pqrs/weakify.h>

@interface KrbnFrontmostApplicationMonitor : NSObject

@property krbn_frontmost_application_monitor_callback callback;
@property void* context;

@end

@implementation KrbnFrontmostApplicationMonitor

- (instancetype)initWithCallback:(krbn_frontmost_application_monitor_callback)callback
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
        if (!self) {
          return;
        }

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

void krbn_frontmost_application_monitor_initialize(krbn_frontmost_application_monitor_objc** monitor,
                                                   krbn_frontmost_application_monitor_callback callback,
                                                   void* context) {
  if (!monitor) {
    NSLog(@"krbn_frontmost_application_monitor_initialize invalid arguments");
    return;
  }
  if (*monitor) {
    // monitor is already initialized.
    return;
  }

  KrbnFrontmostApplicationMonitor* o = [[KrbnFrontmostApplicationMonitor alloc] initWithCallback:callback context:context];
  [o runCallbackWithFrontmostApplication];

  *monitor = (__bridge_retained void*)(o);
}

void krbn_frontmost_application_monitor_terminate(krbn_frontmost_application_monitor_objc** monitor) {
  if (monitor) {
#ifndef __clang_analyzer__
    KrbnFrontmostApplicationMonitor* o = (__bridge_transfer KrbnFrontmostApplicationMonitor*)(*monitor);
    o = nil;
#endif
  }
}
