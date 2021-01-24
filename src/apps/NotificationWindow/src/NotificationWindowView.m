#import "NotificationWindowView.h"

@interface NotificationWindowView ()

@property(weak) IBOutlet NSImageView* closeButton;

@end

@implementation NotificationWindowView

- (void)drawRect:(NSRect)dirtyRect {
  [NSGraphicsContext saveGraphicsState];
  {
    [NSColor.windowBackgroundColor set];
    [[NSBezierPath bezierPathWithRoundedRect:self.bounds xRadius:12 yRadius:12] fill];
  }
  [NSGraphicsContext restoreGraphicsState];
}

- (IBAction)hideWindow:(id)sender {
  [self.window orderOut:self];
}

@end
