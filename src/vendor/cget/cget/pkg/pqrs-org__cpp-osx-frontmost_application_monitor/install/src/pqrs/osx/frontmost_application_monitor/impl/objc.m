// (C) Copyright Takayama Fumihiko 2019.
// Distributed under the Boost Software License, Version 1.0.
// (See http://www.boost.org/LICENSE_1_0.txt)

#import <Cocoa/Cocoa.h>
#import <pqrs/osx/frontmost_application_monitor/impl/objc.h>
#import <pqrs/weakify.h>

@interface PqrsOsxFrontmostApplicationMonitor : NSObject

@property NSOperationQueue* queue;
@property pqrs_osx_frontmost_application_monitor_callback callback;
@property void* context;

@end

@implementation PqrsOsxFrontmostApplicationMonitor

// This method is called in the monitor's dispatcher thread.
- (instancetype)initWithCallback:(pqrs_osx_frontmost_application_monitor_callback)callback
                         context:(void*)context {
  self = [super init];

  if (self) {
    _queue = [NSOperationQueue new];
    _callback = callback;
    _context = context;

    @weakify(self);

    [NSWorkspace.sharedWorkspace.notificationCenter
        addObserverForName:NSWorkspaceDidActivateApplicationNotification
                    object:nil
                     queue:_queue
                usingBlock:^(NSNotification* notification) {
                  // This block is called in `queue_`.

                  @strongify(self);
                  if (!self) {
                    return;
                  }

                  @try {
                    NSRunningApplication* runningApplication = notification.userInfo[NSWorkspaceApplicationKey];
                    [self runCallback:runningApplication];
                  } @catch (NSException* exception) {
                    NSLog(@"[PqrsOsxFrontmostApplicationMonitor NSWorkspaceDidActivateApplicationNotification] error: %@", exception);
                  }
                }];
  }

  return self;
}

// This method is called in the monitor's dispatcher thread.
- (void)dealloc {
  [NSWorkspace.sharedWorkspace.notificationCenter removeObserver:self];

  [_queue waitUntilAllOperationsAreFinished];
}

// This method is called in the monitor's dispatcher thread.
- (void)runCallback:(NSRunningApplication*)runningApplication {
  if (self.callback) {
    @try {
      NSString* bundleIdentifier = runningApplication.bundleIdentifier;
      NSString* path = [[runningApplication.executableURL path] stringByStandardizingPath];

      const char* b = [bundleIdentifier UTF8String];
      const char* p = [path UTF8String];

      self.callback(b ? b : "", p ? p : "", self.context);
    } @catch (NSException* exception) {
      NSLog(@"[PqrsOsxFrontmostApplicationMonitor runCallback] error: %@", exception);
    }
  }
}

// This method is called in the monitor's dispatcher thread.
- (void)runCallbackWithFrontmostApplication {
  [self runCallback:NSWorkspace.sharedWorkspace.frontmostApplication];
}

@end

// This function is called in the monitor's dispatcher thread.
bool pqrs_osx_frontmost_application_monitor_initialize(pqrs_osx_frontmost_application_monitor_objc** monitor,
                                                       pqrs_osx_frontmost_application_monitor_callback callback,
                                                       void* context) {
  if (!monitor) {
    return false;
  }
  if (*monitor) {
    // monitor is already initialized.
    return true;
  }

  PqrsOsxFrontmostApplicationMonitor* o = [[PqrsOsxFrontmostApplicationMonitor alloc] initWithCallback:callback context:context];
  [o runCallbackWithFrontmostApplication];

  *monitor = (__bridge_retained void*)(o);

  return true;
}

// This function is called in the monitor's dispatcher thread.
void pqrs_osx_frontmost_application_monitor_terminate(pqrs_osx_frontmost_application_monitor_objc** monitor) {
  if (monitor) {
#ifndef __clang_analyzer__
    PqrsOsxFrontmostApplicationMonitor* o = (__bridge_transfer PqrsOsxFrontmostApplicationMonitor*)(*monitor);
    o = nil;
#endif
  }
}
