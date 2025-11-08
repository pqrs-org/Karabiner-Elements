import Combine

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

@MainActor
private var previousFingerCount = PreviousFingerCount()

@MainActor
private func staticSetCoreServiceVariable(_ count: FingerCount) {
  struct CoreServiceVariable {
    var name: String
    var value: Int
    var previousValue: PreviousValue
  }

  for gv in [
    CoreServiceVariable(
      name: "multitouch_extension_finger_count_upper_quarter_area",
      value: count.upperQuarterAreaCount,
      previousValue: previousFingerCount.upperQuarterAreaCount
    ),
    CoreServiceVariable(
      name: "multitouch_extension_finger_count_lower_quarter_area",
      value: count.lowerQuarterAreaCount,
      previousValue: previousFingerCount.lowerQuarterAreaCount
    ),
    CoreServiceVariable(
      name: "multitouch_extension_finger_count_left_quarter_area",
      value: count.leftQuarterAreaCount,
      previousValue: previousFingerCount.leftQuarterAreaCount
    ),
    CoreServiceVariable(
      name: "multitouch_extension_finger_count_right_quarter_area",
      value: count.rightQuarterAreaCount,
      previousValue: previousFingerCount.rightQuarterAreaCount
    ),
    CoreServiceVariable(
      name: "multitouch_extension_finger_count_upper_half_area",
      value: count.upperHalfAreaCount,
      previousValue: previousFingerCount.upperHalfAreaCount
    ),
    CoreServiceVariable(
      name: "multitouch_extension_finger_count_lower_half_area",
      value: count.lowerHalfAreaCount,
      previousValue: previousFingerCount.lowerHalfAreaCount
    ),
    CoreServiceVariable(
      name: "multitouch_extension_finger_count_left_half_area",
      value: count.leftHalfAreaCount,
      previousValue: previousFingerCount.leftHalfAreaCount
    ),
    CoreServiceVariable(
      name: "multitouch_extension_finger_count_right_half_area",
      value: count.rightHalfAreaCount,
      previousValue: previousFingerCount.rightHalfAreaCount
    ),
    CoreServiceVariable(
      name: "multitouch_extension_finger_count_total",
      value: count.totalCount,
      previousValue: previousFingerCount.totalCount
    ),
    CoreServiceVariable(
      name: "multitouch_extension_palm_count_upper_half_area",
      value: count.upperHalfAreaPalmCount,
      previousValue: previousFingerCount.upperHalfAreaPalmCount
    ),
    CoreServiceVariable(
      name: "multitouch_extension_palm_count_lower_half_area",
      value: count.lowerHalfAreaPalmCount,
      previousValue: previousFingerCount.lowerHalfAreaPalmCount
    ),
    CoreServiceVariable(
      name: "multitouch_extension_palm_count_left_half_area",
      value: count.leftHalfAreaPalmCount,
      previousValue: previousFingerCount.leftHalfAreaPalmCount
    ),
    CoreServiceVariable(
      name: "multitouch_extension_palm_count_right_half_area",
      value: count.rightHalfAreaPalmCount,
      previousValue: previousFingerCount.rightHalfAreaPalmCount
    ),
    CoreServiceVariable(
      name: "multitouch_extension_palm_count_total",
      value: count.totalPalmCount,
      previousValue: previousFingerCount.totalPalmCount
    ),
  ] where gv.previousValue.value != gv.value {
    libkrbn_core_service_client_async_set_variable(gv.name, Int32(gv.value))

    gv.previousValue.value = gv.value
  }
}

private func callback() {
  Task {
    // sleep until devices are settled.
    try await Task.sleep(nanoseconds: NSEC_PER_SEC)

    Task { @MainActor in
      let status = libkrbn_core_service_client_get_status()

      if status == libkrbn_core_service_client_status_connected {
        MultitouchDeviceManager.shared.setCallback(true)
        libkrbn_core_service_client_async_connect_multitouch_extension()
      } else {
        MultitouchDeviceManager.shared.setCallback(false)
      }

      staticSetCoreServiceVariable(FingerCount())
    }
  }
}

@MainActor
final class MECoreServiceClient {
  static let shared = MECoreServiceClient()

  private var cancellables: Set<AnyCancellable> = []

  init() {
    FingerManager.shared.$fingerCount
      .removeDuplicates()
      .sink { newValue in
        staticSetCoreServiceVariable(newValue)
      }
      .store(in: &cancellables)
  }

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    // Note:
    // The socket file path length must be <= 103 because sizeof(sockaddr_un.sun_path) == 104.
    // So we use the shorten name multitouch_extension_core_service_client -> mt_ext_cs_clnt.
    //
    // Example:
    // `/Library/Application Support/org.pqrs/tmp/user/501/mt_ext_cs_clnt/17d5344d2176c048.sock`

    libkrbn_enable_core_service_client("mt_ext_cs_clnt")

    libkrbn_register_core_service_client_status_changed_callback(callback)

    libkrbn_core_service_client_async_start()
  }
}
