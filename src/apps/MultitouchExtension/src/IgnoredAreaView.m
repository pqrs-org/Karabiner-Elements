#import "IgnoredAreaView.h"
#import "FingerStatusManager.h"
#import "KarabinerKit/KarabinerKit.h"
#import "NotificationKeys.h"
#import "PreferencesController.h"
#import <pqrs/weakify.h>

@interface IgnoredAreaView ()

@property NSArray<FingerStatusEntry*>* fingerStatusEntries;
@property KarabinerKitSmartObserverContainer* observers;

@end

@implementation IgnoredAreaView

- (instancetype)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];

  if (self) {
    _fingerStatusEntries = [NSArray new];
    _observers = [KarabinerKitSmartObserverContainer new];

    @weakify(self);
    {
      NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
      id o = [center addObserverForName:kPhysicalFingerStateChanged
                                 object:nil
                                  queue:[NSOperationQueue mainQueue]
                             usingBlock:^(NSNotification* note) {
                               @strongify(self);
                               if (!self) {
                                 return;
                               }

                               if (self.window.visible) {
                                 [self updateFingerStatusEntries:note.object];
                               }
                             }];
      [_observers addObserver:o notificationCenter:center];
    }
    {
      NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
      id o = [center addObserverForName:kFixedFingerStateChanged
                                 object:nil
                                  queue:[NSOperationQueue mainQueue]
                             usingBlock:^(NSNotification* note) {
                               @strongify(self);
                               if (!self) {
                                 return;
                               }

                               if (self.window.visible) {
                                 [self updateFingerStatusEntries:note.object];
                               }
                             }];
      [_observers addObserver:o notificationCenter:center];
    }
  }

  return self;
}

- (void)updateFingerStatusEntries:(FingerStatusManager*)manager {
  if (manager) {
    self.fingerStatusEntries = [manager copyEntries];

    [self setNeedsDisplay:YES];
  }
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
      const CGFloat DIAMETER = 10.0f;

      if (!e.touchedPhysically && !e.touchedFixed) {
        [[NSColor blackColor] set];
      } else {
        if (e.ignored) {
          [[NSColor blueColor] set];
        } else {
          [[NSColor redColor] set];
        }
      }

      if (e.touchedPhysically) {
        NSRect rect = NSMakeRect(bounds.size.width * e.point.x - DIAMETER / 2,
                                 bounds.size.height * e.point.y - DIAMETER / 2,
                                 DIAMETER,
                                 DIAMETER);
        NSBezierPath* path = [NSBezierPath bezierPathWithOvalInRect:rect];
        [path setLineWidth:2];
        [path stroke];
      }

      if (e.touchedFixed) {
        NSRect rect = NSMakeRect(bounds.size.width * e.point.x - DIAMETER / 4,
                                 bounds.size.height * e.point.y - DIAMETER / 4,
                                 DIAMETER / 2,
                                 DIAMETER / 2);
        NSBezierPath* path = [NSBezierPath bezierPathWithOvalInRect:rect];
        [path setLineWidth:1];
        [path stroke];
      }
    }
  }
  [NSGraphicsContext restoreGraphicsState];
}

- (IBAction)draw:(id)sender {
  [self setNeedsDisplay:YES];
}

@end
