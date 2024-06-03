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
  var upperHalfAreaPalmCount = PreviousValue()
  var lowerHalfAreaPalmCount = PreviousValue()
  var leftHalfAreaPalmCount = PreviousValue()
  var rightHalfAreaPalmCount = PreviousValue()
  var totalPalmCount = PreviousValue()
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
    GrabberVariable(
      name: "multitouch_extension_palm_count_upper_half_area",
      value: count.upperHalfAreaPalmCount,
      previousValue: previousFingerCount.upperHalfAreaPalmCount
    ),
    GrabberVariable(
      name: "multitouch_extension_palm_count_lower_half_area",
      value: count.lowerHalfAreaPalmCount,
      previousValue: previousFingerCount.lowerHalfAreaPalmCount
    ),
    GrabberVariable(
      name: "multitouch_extension_palm_count_left_half_area",
      value: count.leftHalfAreaPalmCount,
      previousValue: previousFingerCount.leftHalfAreaPalmCount
    ),
    GrabberVariable(
      name: "multitouch_extension_palm_count_right_half_area",
      value: count.rightHalfAreaPalmCount,
      previousValue: previousFingerCount.rightHalfAreaPalmCount
    ),
    GrabberVariable(
      name: "multitouch_extension_palm_count_total",
      value: count.totalPalmCount,
      previousValue: previousFingerCount.totalPalmCount
    ),
  ] where gv.previousValue.value != gv.value {
    if sync {
      libkrbn_grabber_client_sync_set_variable(gv.name, Int32(gv.value))
    } else {
      libkrbn_grabber_client_async_set_variable(gv.name, Int32(gv.value))
    }

    gv.previousValue.value = gv.value
  }
}

private func callback() {
  Task {
    // sleep until devices are settled.
    try await Task.sleep(nanoseconds: NSEC_PER_SEC)

    Task { @MainActor in
      let status = libkrbn_grabber_client_get_status()

      if status == libkrbn_grabber_client_status_connected {
        MultitouchDeviceManager.shared.setCallback(true)
        libkrbn_grabber_client_async_connect_multitouch_extension()
      } else {
        MultitouchDeviceManager.shared.setCallback(false)
      }

      staticSetGrabberVariable(FingerCount(), false)
    }
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
  }

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    // Note:
    // The socket file path length must be <= 103 because sizeof(sockaddr_un.sun_path) == 104.
    // So we use the shorten name multitouch_extension_grabber_client -> mt_ext_grb_clnt.
    //
    // Example:
    // `/Library/Application Support/org.pqrs/tmp/user/501/mt_ext_grb_clnt/17d5344d2176c048.sock`

    libkrbn_enable_grabber_client("mt_ext_grb_clnt")

    libkrbn_register_grabber_client_status_changed_callback(callback)
    libkrbn_enqueue_callback(callback)
  }

  @MainActor
  func setGrabberVariable(_ count: FingerCount, _ sync: Bool) {
    staticSetGrabberVariable(count, sync)
  }
}
