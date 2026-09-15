@MainActor
enum RawInputCaptureEmptyMessage {
  static func make(
    deviceSelected: Bool,
    capturing: Bool,
    deviceIsOpen: Bool,
    subject: String,
    capturingEmptyMessage: String,
    localized: AppLocalizationContext.Lookup
  ) -> String {
    if !deviceSelected {
      return localized("event_viewer.capture.select_device", arguments: ["subject": subject])
    }
    if capturing && !deviceIsOpen {
      return localized("event_viewer.capture.waiting")
    }
    if capturing {
      return capturingEmptyMessage
    }
    return localized("event_viewer.capture.start_hint", arguments: ["subject": subject])
  }
}
