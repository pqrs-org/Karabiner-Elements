#import "KarabinerKit/SmartObserverContainer.h"

@interface KarabinerKitSmartObserverContainer ()

@property NSMutableArray* notificationCenterObservers;

@end

@implementation KarabinerKitSmartObserverContainer

- (instancetype)init {
  self = [super init];

  if (self) {
    _notificationCenterObservers = [NSMutableArray new];
  }

  return self;
}

- (void)dealloc {
  for (id observer in self.notificationCenterObservers) {
    [[NSNotificationCenter defaultCenter] removeObserver:observer];
  }
}

- (void)addNotificationCenterObserver:(id)observer {
  [self.notificationCenterObservers addObject:observer];
}

@end
