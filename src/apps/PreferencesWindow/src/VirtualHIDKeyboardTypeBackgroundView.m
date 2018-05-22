#import "VirtualHIDKeyboardTypeBackgroundView.h"

@implementation VirtualHIDKeyboardTypeBackgroundView

- (void)drawRect:(NSRect)dirtyRect {
  [NSGraphicsContext saveGraphicsState];
  {
    NSRect bounds = [self bounds];

    // Draw background
    [[[NSColor redColor] colorWithAlphaComponent:0.2f] set];
    [[NSBezierPath bezierPathWithRoundedRect:bounds xRadius:10.0f yRadius:10.0f] fill];

    // Draw border
    [[[NSColor redColor] colorWithAlphaComponent:0.8f] set];
    NSBezierPath* path = [NSBezierPath bezierPathWithRoundedRect:bounds xRadius:10.0f yRadius:10.0f];
    [path setLineWidth:5.0f];
    [path stroke];
  }
  [NSGraphicsContext restoreGraphicsState];
}

@end
