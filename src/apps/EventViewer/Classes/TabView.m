#import "TabView.h"

@implementation TabView

- (BOOL)acceptsFirstResponder {
  // Disable to become key view in order to avoid
  // grabbing control-tab when Full Keyboard Access is enabled.

  return NO;
}

@end
