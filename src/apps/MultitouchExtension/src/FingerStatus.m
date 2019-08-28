#import "FingerStatus.h"

@interface FingerStatusItem : NSObject

@property int identifier;
@property BOOL active;

@end

@implementation FingerStatusItem
@end

@interface FingerStatus ()

@property NSMutableArray<FingerStatusItem*>* items;

@end

@implementation FingerStatus

- (instancetype)init {
  self = [super init];

  if (self) {
    _items = [NSMutableArray new];
  }

  return self;
}

- (void)clear {
  [self.items removeAllObjects];
}

- (void)add:(int)identifier active:(BOOL)active {
  FingerStatusItem* item = [FingerStatusItem new];
  item.identifier = identifier;
  item.active = active;

  [self.items addObject:item];
}

- (BOOL)isActive:(int)identifier {
  for (FingerStatusItem* item in self.items) {
    if (item.identifier == identifier) {
      return item.active;
    }
  }
  return NO;
}

@end
