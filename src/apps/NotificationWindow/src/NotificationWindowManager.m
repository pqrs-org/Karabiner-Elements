#import "NotificationWindowManager.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationWindowView.h"
#import "libkrbn/libkrbn.h"
#import <pqrs/weakify.h>

@interface NotificationWindowManager ()

@property(copy) NSMutableArray* windowControllers;

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

    [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidChangeScreenParametersNotification
                                                      object:nil
                                                       queue:[NSOperationQueue mainQueue]
                                                  usingBlock:^(NSNotification* note) {
                                                    [self updateWindows];
                                                  }];

    [self updateWindows];

    libkrbn_enable_notification_message_json_file_monitor(staticCallback,
                                                          (__bridge void*)(self));
  }

  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  libkrbn_disable_notification_message_json_file_monitor();
}

- (void)updateWindows {
  NSArray* screens = NSScreen.screens;

  if (self.windowControllers.count != screens.count) {
    [self.windowControllers removeAllObjects];

    for (NSScreen* screen in screens) {
      NSWindowController* controller = [[NSWindowController alloc] initWithWindowNibName:@"NotificationWindow"];
      [self setupWindow:controller.window screen:screen];
      [self.windowControllers addObject:controller];
    }
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
    [self setText:json[@"body"]];
  });
}

@end
