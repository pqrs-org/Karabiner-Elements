#import "IgnoredAreaView.h"
#import "FingerStatusManager.h"
#import "NotificationKeys.h"
#import "PreferencesController.h"
#import <pqrs/weakify.h>

@interface IgnoredAreaView ()

@property NSArray<FingerStatusEntry*>* fingerStatusEntries;

@end

@implementation IgnoredAreaView

- (instancetype)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];

  if (self) {
    _fingerStatusEntries = [NSArray new];

    @weakify(self);
    [[NSNotificationCenter defaultCenter] addObserverForName:kPhysicalFingerStateChanged
                                                      object:nil
                                                       queue:[NSOperationQueue mainQueue]
                                                  usingBlock:^(NSNotification* note) {
                                                    @strongify(self);
                                                    if (!self) {
                                                      return;
                                                    }

                                                    FingerStatusManager* m = note.object;
                                                    self.fingerStatusEntries = [m copyEntries];

                                                    [self setNeedsDisplay:YES];
                                                  }];
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
    NSRect targetArea = [PreferencesController makeTargetArea];

    [[NSColor grayColor] set];
    [[NSBezierPath bezierPathWithRoundedRect:NSMakeRect(bounds.size.width * targetArea.origin.x,
                                                        bounds.size.height * targetArea.origin.y,
                                                        bounds.size.width * targetArea.size.width,
                                                        bounds.size.height * targetArea.size.height)
                                     xRadius:10
                                     yRadius:10] fill];

    // Draw fingers
    for (FingerStatusEntry* e in self.fingerStatusEntries) {
      if (!e.touchedPhysically) {
        continue;
      }

      const int DIAMETER = 10;

      if (e.ignored) {
        [[NSColor blueColor] set];
      } else {
        [[NSColor redColor] set];
      }
      NSRect rect = NSMakeRect(bounds.size.width * e.point.x - DIAMETER / 2,
                               bounds.size.height * e.point.y - DIAMETER / 2,
                               DIAMETER,
                               DIAMETER);
      NSBezierPath* path = [NSBezierPath bezierPathWithOvalInRect:rect];
      [path setLineWidth:2];
      [path stroke];
    }
  }
  [NSGraphicsContext restoreGraphicsState];
}

- (IBAction)draw:(id)sender {
  [self setNeedsDisplay:YES];
}

@end
