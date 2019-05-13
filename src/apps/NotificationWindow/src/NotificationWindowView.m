#import "NotificationWindowView.h"

@implementation NotificationWindowView

- (void)drawRect:(NSRect)dirtyRect {
  [NSGraphicsContext saveGraphicsState];
  {
    [NSColor.windowBackgroundColor set];
    [[NSBezierPath bezierPathWithRoundedRect:self.bounds xRadius:12 yRadius:12] fill];
  }
  [NSGraphicsContext restoreGraphicsState];
}

@end
