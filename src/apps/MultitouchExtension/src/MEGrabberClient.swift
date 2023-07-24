private class PreviousValue {
  var value: Int = -1
}

private class PreviousFingerCount {
  var upperQuarterAreaCount = PreviousValue()
  var lowerQuarterAreaCount = PreviousValue()
  var leftQuarterAreaCount = PreviousValue()
  var rightQuarterAreaCount = PreviousValue()
  var upperHalfAreaCount = PreviousValue()
  var lowerHalfAreaCount = PreviousValue()
  var leftHalfAreaCount = PreviousValue()
  var rightHalfAreaCount = PreviousValue()
  var totalCount = PreviousValue()
}

private var previousFingerCount = PreviousFingerCount()

private func staticSetGrabberVariable(_ count: FingerCount, _ sync: Bool) {
  struct GrabberVariable {
    var name: String
    var value: Int
    var previousValue: PreviousValue
  }

  for gv in [
    GrabberVariable(
      name: "multitouch_extension_finger_count_upper_quarter_area",
      value: count.upperQuarterAreaCount,
      previousValue: previousFingerCount.upperQuarterAreaCount
    ),
    GrabberVariable(
      name: "multitouch_extension_finger_count_lower_quarter_area",
      value: count.lowerQuarterAreaCount,
      previousValue: previousFingerCount.lowerQuarterAreaCount
    ),
    GrabberVariable(
      name: "multitouch_extension_finger_count_left_quarter_area",
      value: count.leftQuarterAreaCount,
      previousValue: previousFingerCount.leftQuarterAreaCount
    ),
    GrabberVariable(
      name: "multitouch_extension_finger_count_right_quarter_area",
      value: count.rightQuarterAreaCount,
      previousValue: previousFingerCount.rightQuarterAreaCount
    ),
    GrabberVariable(
      name: "multitouch_extension_finger_count_upper_half_area",
      value: count.upperHalfAreaCount,
      previousValue: previousFingerCount.upperHalfAreaCount
    ),
    GrabberVariable(
      name: "multitouch_extension_finger_count_lower_half_area",
      value: count.lowerHalfAreaCount,
      previousValue: previousFingerCount.lowerHalfAreaCount
    ),
    GrabberVariable(
      name: "multitouch_extension_finger_count_left_half_area",
      value: count.leftHalfAreaCount,
      previousValue: previousFingerCount.leftHalfAreaCount
    ),
    GrabberVariable(
      name: "multitouch_extension_finger_count_right_half_area",
      value: count.rightHalfAreaCount,
      previousValue: previousFingerCount.rightHalfAreaCount
    ),
    GrabberVariable(
      name: "multitouch_extension_finger_count_total",
      value: count.totalCount,
      previousValue: previousFingerCount.totalCount
    ),
  ] {
    if gv.previousValue.value != gv.value {
      if sync {
        libkrbn_grabber_client_sync_set_variable(gv.name, Int32(gv.value))
      } else {
        libkrbn_grabber_client_async_set_variable(gv.name, Int32(gv.value))
      }

      gv.previousValue.value = gv.value
    }
  }
}

private func enable() {
  Task { @MainActor in
    // sleep until devices are settled.
    try await Task.sleep(nanoseconds: NSEC_PER_SEC)

    MultitouchDeviceManager.shared.setCallback(true)

    staticSetGrabberVariable(FingerCount(), false)
  }
}

private func disable() {
  Task { @MainActor in
    MultitouchDeviceManager.shared.setCallback(false)
  }
}

final class MEGrabberClient {
  static let shared = MEGrabberClient()

  init() {
    NotificationCenter.default.addObserver(
      forName: FingerState.fingerStateChanged,
      object: nil,
      queue: .main
    ) { _ in
      Task { @MainActor in
        staticSetGrabberVariable(FingerManager.shared.fingerCount, false)
      }
    }

    libkrbn_enable_grabber_client(
      enable,
      disable,
      disable)
  }

  @MainActor
  func setGrabberVariable(_ count: FingerCount, _ sync: Bool) {
    staticSetGrabberVariable(count, sync)
  }
}
