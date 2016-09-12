#import "EventViewerApplication.h"
#import "EventQueue.h"

@interface EventViewerApplication ()

@property(weak) IBOutlet EventQueue* eventQueue;

@end

@implementation EventViewerApplication

- (void)sendEvent:(NSEvent*)event {
  [self.eventQueue pushFromNSApplication:event];

  [super sendEvent:event];
}

@end
