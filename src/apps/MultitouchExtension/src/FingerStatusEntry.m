#import "FingerStatusEntry.h"

@implementation FingerStatusEntry

- (instancetype)initWithDevice:(MTDeviceRef)device
                    identifier:(int)identifier {
  self = [super init];

  if (self) {
    _device = device;
    _identifier = identifier;
    _frame = 0;
    _point = NSMakePoint(0, 0);
    _touchedPhysically = NO;
    _touchedFixed = NO;
    _ignored = YES;
    _delayTimer = nil;
  }

  return self;
}

- (instancetype)copyWithZone:(NSZone*)zone {
  printf("make copy\n");

  FingerStatusEntry* e = [[FingerStatusEntry alloc] initWithDevice:self.device identifier:self.identifier];

  e.frame = self.frame;
  e.point = self.point;
  e.touchedPhysically = self.touchedPhysically;
  e.touchedFixed = self.touchedFixed;
  e.ignored = self.ignored;
  // e.delayTimer is nil

  return e;
}

@end
