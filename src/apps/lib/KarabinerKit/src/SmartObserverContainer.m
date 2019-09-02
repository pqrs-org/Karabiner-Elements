#import "KarabinerKit/SmartObserverContainer.h"

//
// KarabinerKitSmartObserverContainerEntry
//

@interface KarabinerKitSmartObserverContainerEntry : NSObject

@property id observer;
@property NSNotificationCenter* notificationCenter;

@end

@implementation KarabinerKitSmartObserverContainerEntry

- (instancetype)initWithObserver:(id)observer
              notificationCenter:(NSNotificationCenter*)notificationCenter {
  self = [super init];

  if (self) {
    _observer = observer;
    _notificationCenter = notificationCenter;
  }

  return self;
}

@end

//
// KarabinerKitSmartObserverContainer
//

@interface KarabinerKitSmartObserverContainer ()

@property NSMutableArray<KarabinerKitSmartObserverContainerEntry*>* entries;

@end

@implementation KarabinerKitSmartObserverContainer

- (instancetype)init {
  self = [super init];

  if (self) {
    _entries = [NSMutableArray new];
  }

  return self;
}

- (void)dealloc {
  for (KarabinerKitSmartObserverContainerEntry* e in self.entries) {
    [e.notificationCenter removeObserver:e.observer];
  }
}

- (void)addObserver:(id)observer
    notificationCenter:(NSNotificationCenter*)notificationCenter {
  KarabinerKitSmartObserverContainerEntry* e =
      [[KarabinerKitSmartObserverContainerEntry alloc] initWithObserver:observer
                                                     notificationCenter:notificationCenter];

  [self.entries addObject:e];
}

@end
