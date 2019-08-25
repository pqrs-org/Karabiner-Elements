#import "IgnoredAreaView.h"
#import "PreferencesController.h"

@interface Finger : NSObject

@property NSPoint point;
@property BOOL ignored;

@end

@implementation Finger
@end

@interface IgnoredAreaView ()

@property NSMutableArray* fingers;

@end

@implementation IgnoredAreaView

+ (NSRect)getTargetArea {
  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  double top = [[defaults stringForKey:@"ignoredAreaTop"] doubleValue] / 100;
  double bottom = [[defaults stringForKey:@"ignoredAreaBottom"] doubleValue] / 100;
  double left = [[defaults stringForKey:@"ignoredAreaLeft"] doubleValue] / 100;
  double right = [[defaults stringForKey:@"ignoredAreaRight"] doubleValue] / 100;

  return NSMakeRect(left,
                    bottom,
                    (1.0 - left - right),
                    (1.0 - top - bottom));
}

- (instancetype)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];

  if (self) {
    _fingers = [NSMutableArray new];
  }

  return self;
}

- (void)drawRect:(NSRect)dirtyRect {
  [NSGraphicsContext saveGraphicsState];
  {
    NSRect bounds = [self bounds];

    // Draw bounds
    [[[NSColor grayColor] colorWithAlphaComponent:0.5] set];
    [[NSBezierPath bezierPathWithRoundedRect:bounds xRadius:10 yRadius:10] fill];

    // Draw target area
    NSRect targetArea = [IgnoredAreaView getTargetArea];

    [[NSColor grayColor] set];
    [[NSBezierPath bezierPathWithRoundedRect:NSMakeRect(bounds.size.width * targetArea.origin.x,
                                                        bounds.size.height * targetArea.origin.y,
                                                        bounds.size.width * targetArea.size.width,
                                                        bounds.size.height * targetArea.size.height)
                                     xRadius:10
                                     yRadius:10] fill];

    // Draw fingers
    for (Finger* finger in self.fingers) {
      enum {
        DIAMETER = 10,
      };

      if (finger.ignored) {
        [[NSColor blueColor] set];
      } else {
        [[NSColor redColor] set];
      }
      NSRect rect = NSMakeRect(bounds.size.width * finger.point.x - DIAMETER / 2,
                               bounds.size.height * finger.point.y - DIAMETER / 2,
                               DIAMETER,
                               DIAMETER);
      NSBezierPath* path = [NSBezierPath bezierPathWithOvalInRect:rect];
      [path setLineWidth:2];
      [path stroke];
    }
  }
  [NSGraphicsContext restoreGraphicsState];
}

- (void)clearFingers {
  [self.fingers removeAllObjects];

  [self setNeedsDisplay:YES];
}

- (void)addFinger:(NSPoint)point ignored:(BOOL)ignored {
  Finger* finger = [Finger new];
  finger.point = point;
  finger.ignored = ignored;

  [self.fingers addObject:finger];

  [self setNeedsDisplay:YES];
}

+ (BOOL)isIgnoredArea:(NSPoint)point {
  NSRect targetArea = [IgnoredAreaView getTargetArea];
  return !NSPointInRect(point, targetArea);
}

- (IBAction)draw:(id)sender {
  [self setNeedsDisplay:YES];
}

@end
