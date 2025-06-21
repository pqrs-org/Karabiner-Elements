class FingerState: Identifiable {
  public let id = UUID()

  //
  // Unique keys
  //

  var device: MTDevice
  var identifier = 0

  //
  // Variables
  //

  var frame = 0

  var point = NSPoint.zero

  var size = 0.0

  // True if the finger is touched physically.
  var touchedPhysically = false
  var touchedPhysicallyAt = Date()

  // True if the finger is touched continuously for the specified time. (finger touch detection delay)
  var touchedFixed = false

  // True while the finger has never entered the valid area.
  var ignored = true

  // True if the palm is larger than the threshold
  var palmed = false

  var contactFrameArrivedAt = Date()

  //
  // Methods
  //

  init(device: MTDevice, identifier: Int) {
    self.device = device
    self.identifier = identifier
  }

  func updateTouchedFixed(now: Date) -> Bool {
    let delay =
      touchedPhysically
      ? UserSettings.shared.delayBeforeTurnOn
      : UserSettings.shared.delayBeforeTurnOff

    if touchedFixed != touchedPhysically {
      let elapsedMs = Int(now.timeIntervalSince(touchedPhysicallyAt) * 1000)
      if elapsedMs >= delay {
        touchedFixed = touchedPhysically
        return true
      }
    }

    return false
  }
}
