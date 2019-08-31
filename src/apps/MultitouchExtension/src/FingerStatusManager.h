// -*- mode: objective-c -*-

@import Cocoa;
#import "MultitouchPrivate.h"

@interface FingerStatusManager : NSObject

- (void)update:(MTDeviceRef)device
          data:(Finger*)data
       fingers:(int)fingers
     timestamp:(double)timestamp
         frame:(int)frame;

- (void)debugDump;

@end
