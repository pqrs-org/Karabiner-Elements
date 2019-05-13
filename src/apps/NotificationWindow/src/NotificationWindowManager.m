#import "NotificationWindowManager.h"
#import "NotificationWindowView.h"
#import <pqrs/weakify.h>

@interface NotificationWindowManager ()

@property(copy) NSMutableArray* windowControllers;
@end

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
  }

  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
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

@end
