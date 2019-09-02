#import "MultitouchDeviceManager.h"
#import "FingerStatusManager.h"

//
// Multitouch callback
//

static int callback(MTDeviceRef device,
                    Finger* data,
                    int fingers,
                    double timestamp,
                    int frame) {
  if (!data) {
    fingers = 0;
  }

  if (device) {
    FingerStatusManager* manager = [FingerStatusManager sharedFingerStatusManager];
    [manager update:device
               data:data
            fingers:fingers
          timestamp:timestamp
              frame:frame];
  }

  return 0;
}

@interface MultitouchDeviceManager ()

@property(copy) NSArray* devices;

@end

@implementation MultitouchDeviceManager

- (instancetype)init {
  self = [super init];

  if (self) {
    _devices = [NSMutableArray new];
  }

  return self;
}

+ (instancetype)sharedMultitouchDeviceManager {
  static dispatch_once_t once;
  static MultitouchDeviceManager* manager;

  dispatch_once(&once, ^{
    manager = [MultitouchDeviceManager new];
  });

  return manager;
}

- (void)setCallback:(BOOL)set {
  @synchronized(self) {
    // ------------------------------------------------------------
    // unset callback (even if set is YES.)
    if (self.devices) {
      for (NSUInteger i = 0; i < [self.devices count]; ++i) {
        MTDeviceRef device = (__bridge MTDeviceRef)(self.devices[i]);
        if (!device) continue;

        MTDeviceStop(device, 0);
        MTUnregisterContactFrameCallback(device, callback);
      }

      self.devices = [NSArray new];
    }

    // ------------------------------------------------------------
    // set callback if needed
    if (set) {
      self.devices = (NSArray*)CFBridgingRelease(MTDeviceCreateList());
      if (self.devices) {
        for (NSUInteger i = 0; i < [self.devices count]; ++i) {
          MTDeviceRef device = (__bridge MTDeviceRef)(self.devices[i]);
          if (!device) continue;

          MTRegisterContactFrameCallback(device, callback);
          MTDeviceStart(device, 0);
        }
      }
    }
  }
}

@end
