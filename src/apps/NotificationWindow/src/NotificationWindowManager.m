#import "NotificationWindowManager.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationWindowView.h"
#import "libkrbn/libkrbn.h"
#import <pqrs/weakify.h>

@interface NotificationWindowManager ()

@property(copy) NSMutableArray* windowControllers;
@property KarabinerKitSmartObserverContainer* observers;

- (void)callback:(NSString*)filePath;

@end

static void staticCallback(const char* filePath,
                           void* context) {
  NotificationWindowManager* p = (__bridge NotificationWindowManager*)(context);
  if (p && filePath) {
    [p callback:[NSString stringWithUTF8String:filePath]];
  }
}

@implementation NotificationWindowManager

- (instancetype)init {
  self = [super init];

  if (self) {
    _text = @"";
    _windowControllers = [NSMutableArray new];
    _observers = [KarabinerKitSmartObserverContainer new];

    @weakify(self);

    {
      NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
      id o = [center addObserverForName:NSApplicationDidChangeScreenParametersNotification
                                 object:nil
                                  queue:[NSOperationQueue mainQueue]
                             usingBlock:^(NSNotification* note) {
                               @strongify(self);
                               if (!self) {
                                 return;
                               }

                               [self updateWindows];
                             }];
      [_observers addObserver:o notificationCenter:center];
    }

    [self updateWindows];

    libkrbn_enable_notification_message_json_file_monitor(staticCallback,
                                                          (__bridge void*)(self));
  }

  return self;
}

- (void)dealloc {
  libkrbn_disable_notification_message_json_file_monitor();
}

- (void)updateWindows {
  NSArray* screens = NSScreen.screens;

  // The old window sometimes does not deallocated properly when screen count is decreased.
  // Thus, we hide the window before clear windowControllers.
  for (NSWindowController* c in self.windowControllers) {
    [c.window orderOut:self];
  }
  [self.windowControllers removeAllObjects];

  for (NSScreen* screen in screens) {
    NSWindowController* controller = [[NSWindowController alloc] initWithWindowNibName:@"NotificationWindow"];
    [self setupWindow:controller.window screen:screen];
    [self.windowControllers addObject:controller];
  }

  [self setNotificationText];
}

- (void)setupWindow:(NSWindow*)window
             screen:(NSScreen*)screen {
  [window setBackgroundColor:NSColor.clearColor];
  [window setOpaque:NO];
  [window setLevel:NSStatusWindowLevel];
  [window setIgnoresMouseEvents:YES];
  [window setCollectionBehavior:(NSWindowCollectionBehaviorCanJoinAllSpaces |
                                 NSWindowCollectionBehaviorIgnoresCycle |
                                 NSWindowCollectionBehaviorStationary)];

  NSRect screenFrame = screen.visibleFrame;
  NSRect windowFrame = window.frame;
  int margin = 10;
  window.frameOrigin = NSMakePoint(
      screenFrame.origin.x + screenFrame.size.width - windowFrame.size.width - margin,
      screenFrame.origin.y + margin);
}

- (void)setNotificationText {
  NSString* text = self.text;

  for (NSWindowController* controller in self.windowControllers) {
    if (text.length == 0) {
      [controller.window orderOut:self];
    }

    NotificationWindowView* view = controller.window.contentView;
    view.text.stringValue = text;

    if (text.length > 0) {
      [controller.window orderFront:self];
    }
  }
}

- (void)setText:(NSString*)text {
  if (!text) {
    return;
  }

  _text = text;

  @weakify(self);
  dispatch_async(dispatch_get_main_queue(), ^{
    @strongify(self);
    if (!self) {
      return;
    }

    [self setNotificationText];
  });
}

- (void)callback:(NSString*)filePath {
  @weakify(self);
  dispatch_async(dispatch_get_main_queue(), ^{
    @strongify(self);
    if (!self) {
      return;
    }

    NSDictionary* json = [KarabinerKitJsonUtility loadFile:filePath];
    if (json) {
      NSObject* body = json[@"body"];
      if ([body isKindOfClass:NSString.class]) {
        [self setText:(NSString*)body];
      }
    }
  });
}

@end
