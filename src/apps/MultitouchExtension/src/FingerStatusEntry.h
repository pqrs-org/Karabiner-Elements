// -*- mode: objective-c -*-

@import Cocoa;
#import "MultitouchPrivate.h"

@interface FingerStatusEntry : NSObject <NSCopying>

//
// Unique keys
//

@property MTDeviceRef device;
@property int identifier;

//
// Variables
//

@property int frame;
@property NSPoint point;
@property BOOL touchedPhysically;
@property BOOL touchedFixed;
@property BOOL ignored;
@property NSTimer* delayTimer;

//
// Methods
//

- (instancetype)initWithDevice:(MTDeviceRef)device
                    identifier:(int)identifier;

@end
