enum FingerStatusEntryTimerMode {
  case none
  case touched
  case untouched
}

class FingerStatusEntry: Identifiable {
  public var id = UUID()

  //
  // Unique keys
  //

  var device: MTDevice
  var identifier = 0

  //
  // Variables
  //

  var frame = 0

  var point = NSMakePoint(0, 0)

  // True if the finger is touched physically.
  var touchedPhysically = false

  // True if the finger is touched continuously for the specified time. (finger touch detection delay)
  var touchedFixed = false

  // True while the finger has never entered the valid area.
  var ignored = true

  /*
@property NSTimer* delayTimer;
@property enum FingerStatusEntryTimerMode timerMode;
*/

  //
  // Methods
  //

  init(device: MTDevice, identifier: Int) {
    self.device = device
    self.identifier = identifier
    /*
    _delayTimer = nil
    _timerMode = FingerStatusEntryTimerModeNone
    */
  }
}
