enum FingerStatusEntryTimerMode {
  case none
  case touched
  case untouched
}

class FingerStatusEntry {
  //
  // Unique keys
  //

  var device: MTDeviceRef
  var identifier = 0

  //
  // Variables
  //

  var frame = 0
  var point = NSMakePoint(0, 0)
  /*
// True if the finger is touched physically.
@property BOOL touchedPhysically;
// True if the finger is touched continuously for the specified time. (finger touch detection delay)
@property BOOL touchedFixed;
// True while the finger has never entered the valid area.
@property BOOL ignored;
@property NSTimer* delayTimer;
@property enum FingerStatusEntryTimerMode timerMode;
*/

  //
  // Methods
  //

  init(device: MTDeviceRef, identifier: Int) {
    self.device = device
    self.identifier = identifier
    /*
    _touchedPhysically = NO
    _touchedFixed = NO
    _ignored = YES
    _delayTimer = nil
    _timerMode = FingerStatusEntryTimerModeNone
    */
  }

  /*
- (instancetype)copyWithZone:(NSZone*)zone {
  FingerStatusEntry* e = [[FingerStatusEntry alloc] initWithDevice:self.device identifier:self.identifier];

  e.frame = self.frame;
  e.point = self.point;
  e.touchedPhysically = self.touchedPhysically;
  e.touchedFixed = self.touchedFixed;
  e.ignored = self.ignored;
  // e.delayTimer is nil
  e.timerMode = FingerStatusEntryTimerModeNone;

  return e;
}

@end
*/
}
