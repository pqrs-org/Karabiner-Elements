enum RawInputCaptureEmptyMessage {
  static func make(
    deviceSelected: Bool,
    capturing: Bool,
    deviceIsOpen: Bool,
    subject: String,
    capturingEmptyMessage: String
  ) -> String {
    if !deviceSelected {
      return "Select the device whose \(subject) you want to inspect."
    }
    if capturing && !deviceIsOpen {
      return "Waiting for device access..."
    }
    if capturing {
      return capturingEmptyMessage
    }
    return "Press Start capture to begin capturing \(subject)."
  }
}
