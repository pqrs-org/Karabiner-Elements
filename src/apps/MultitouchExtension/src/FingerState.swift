class FingerState: Identifiable {
  static let fingerStateChanged = Notification.Name("fingerStateChanged")

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

  var point = NSZeroPoint

  // True if the finger is touched physically.
  var touchedPhysically = false

  // True if the finger is touched continuously for the specified time. (finger touch detection delay)
  var touchedFixed = false

  // True while the finger has never entered the valid area.
  var ignored = true

  var delayTask: Task<(), Never>?

  enum DelayMode {
    case none
    case touched
    case untouched
  }
  private var delayMode = DelayMode.none

  //
  // Methods
  //

  init(device: MTDevice, identifier: Int) {
    self.device = device
    self.identifier = identifier
  }

  func setDelayTask(mode: DelayMode) {
    if delayMode != mode {
      delayTask?.cancel()

      let delay =
        mode == .touched
        ? UserSettings.shared.delayBeforeTurnOn
        : UserSettings.shared.delayBeforeTurnOff

      delayMode = mode
      delayTask = Task { @MainActor in
        do {
          try await Task.sleep(nanoseconds: UInt64(delay) * NSEC_PER_MSEC)

          if Task.isCancelled {
            return
          }

          switch delayMode {
          case .touched:
            touchedFixed = true

          case .untouched:
            touchedFixed = false

          case .none:
            // Do nothing
            break
          }

          NotificationCenter.default.post(name: FingerState.fingerStateChanged, object: nil)
        } catch {
          print("cancelled")
        }
      }
    }
  }
}
