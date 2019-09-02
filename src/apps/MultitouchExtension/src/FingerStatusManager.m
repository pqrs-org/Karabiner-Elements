#import "FingerStatusManager.h"
#import "NotificationKeys.h"
#import "PreferencesController.h"
#import "PreferencesKeys.h"
#import <pqrs/weakify.h>

@interface FingerStatusManager ()

@property NSMutableArray<FingerStatusEntry*>* entries;
@end

@implementation FingerStatusManager

- (instancetype)init {
  self = [super init];

  if (self) {
    _entries = [NSMutableArray new];
  }

  return self;
}

- (void)update:(MTDeviceRef)device
          data:(Finger*)data
       fingers:(int)fingers
     timestamp:(double)timestamp
         frame:(int)frame {
  BOOL callFixedFingerStateChanged = NO;

  @synchronized(self) {
    //
    // Update physical touched fingers
    //

    for (int i = 0; i < fingers; ++i) {
      int identifier = data[i].identifier;

      FingerStatusEntry* e = [self findEntry:device identifier:identifier];
      if (!e) {
        e = [[FingerStatusEntry alloc] initWithDevice:device identifier:identifier];
        [self.entries addObject:e];
      }

      e.frame = frame;
      e.point = NSMakePoint(data[i].normalized.position.x, data[i].normalized.position.y);

      // Note:
      // Once the point in targetArea, keep `ignored == NO`.
      if (e.ignored) {
        NSRect targetArea = [PreferencesController makeTargetArea];
        if (NSPointInRect(e.point, targetArea)) {
          e.ignored = NO;
          callFixedFingerStateChanged = YES;
        }
      }

      // state values:
      //   4: touched
      //   1-3,5-7: near
      BOOL touched = NO;
      if (data[i].state == 4) {
        touched = YES;
      } else {
        touched = NO;
      }

      if (e.touchedPhysically != touched) {
        e.touchedPhysically = touched;

        [self setFingerStatusEntryDelayTimer:e touched:touched];
      }
    }

    //
    // Update physical untouched fingers
    //

    for (FingerStatusEntry* e in self.entries) {
      if (e.device == device &&
          e.frame != frame &&
          e.touchedPhysically) {
        e.touchedPhysically = NO;

        [self setFingerStatusEntryDelayTimer:e touched:NO];
      }
    }
  }

  [[NSNotificationCenter defaultCenter] postNotificationName:kPhysicalFingerStateChanged
                                                      object:self];

  if (callFixedFingerStateChanged) {
    [[NSNotificationCenter defaultCenter] postNotificationName:kFixedFingerStateChanged
                                                        object:self];
  }
}

// Note: This method is called in @synchronized(self)
- (FingerStatusEntry*)findEntry:(MTDeviceRef)device
                     identifier:(int)identifier {
  for (FingerStatusEntry* e in self.entries) {
    if (e.device == device &&
        e.identifier == identifier) {
      return e;
    }
  }

  return nil;
}

// Note: This method is called in @synchronized(self)
- (void)setFingerStatusEntryDelayTimer:(FingerStatusEntry*)entry
                               touched:(BOOL)touched {
  if (touched == entry.touchedFixed) {
    [entry.delayTimer invalidate];
  } else {
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];

    double delay = 0;
    if (touched) {
      delay = [defaults integerForKey:kDelayBeforeTurnOn];
    } else {
      delay = [defaults integerForKey:kDelayBeforeTurnOff];
    }

    @weakify(self);
    @weakify(entry);
    entry.delayTimer = [NSTimer timerWithTimeInterval:delay / 1000.0
                                              repeats:NO
                                                block:^(NSTimer* timer) {
                                                  @strongify(self);
                                                  if (!self) {
                                                    return;
                                                  }

                                                  @strongify(entry);
                                                  if (!entry) {
                                                    return;
                                                  }

                                                  @synchronized(self) {
                                                    entry.touchedFixed = touched;

                                                    if (!touched) {
                                                      [self.entries removeObject:entry];
                                                    }
                                                  }

                                                  [[NSNotificationCenter defaultCenter] postNotificationName:kFixedFingerStateChanged
                                                                                                      object:self];
                                                }];
    [[NSRunLoop mainRunLoop] addTimer:entry.delayTimer forMode:NSRunLoopCommonModes];
  }
}

- (NSArray<FingerStatusEntry*>*)copyEntries {
  @synchronized(self) {
    return [[NSArray alloc] initWithArray:self.entries copyItems:YES];
  }
}

- (NSUInteger)getTouchedFixedFingerCount {
  @synchronized(self) {
    NSUInteger count = 0;

    for (FingerStatusEntry* e in self.entries) {
      if (e.ignored) {
        continue;
      }

      if (!e.touchedFixed) {
        continue;
      }

      ++count;
    }

    return count;
  }
}

@end
