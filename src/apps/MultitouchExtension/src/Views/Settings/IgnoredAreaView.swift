import SwiftUI

struct IgnoredAreaView: View {
  /*
- (void)drawRect:(NSRect)dirtyRect {
  [NSGraphicsContext saveGraphicsState];
  {
    // Draw fingers
    for (FingerStatusEntry* e in self.fingerStatusEntries) {
      const CGFloat DIAMETER = 10.0f;

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
*/

  var body: some View {
    ZStack {
    }
  }
}
