// -*- mode: objective-c -*-

@import Cocoa;
#import "MultitouchPrivate.h"

enum FingerStatusEntryTimerMode {
  FingerStatusEntryTimerModeNone,
  FingerStatusEntryTimerModeTouched,
  FingerStatusEntryTimerModeUntouched,
};

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
// True if the finger is touched physically.
@property BOOL touchedPhysically;
// True if the finger is touched continuously for the specified time. (finger touch detection delay)
@property BOOL touchedFixed;
// True while the finger has never entered the valid area.
@property BOOL ignored;
@property NSTimer* delayTimer;
@property enum FingerStatusEntryTimerMode timerMode;

//
// Methods
//

- (instancetype)initWithDevice:(MTDeviceRef)device
                    identifier:(int)identifier;

@end
