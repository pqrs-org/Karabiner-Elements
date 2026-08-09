import Combine

@MainActor
private func staticSetCoreServiceVariables(_ count: FingerCount) {
  let requested = krbn_core_service_async_set_variables(
    krbn_multitouch_extension_variables(
      finger_count_upper_quarter_area: Int32(count.upperQuarterAreaCount),
      finger_count_lower_quarter_area: Int32(count.lowerQuarterAreaCount),
      finger_count_left_quarter_area: Int32(count.leftQuarterAreaCount),
      finger_count_right_quarter_area: Int32(count.rightQuarterAreaCount),
      finger_count_upper_half_area: Int32(count.upperHalfAreaCount),
      finger_count_lower_half_area: Int32(count.lowerHalfAreaCount),
      finger_count_left_half_area: Int32(count.leftHalfAreaCount),
      finger_count_right_half_area: Int32(count.rightHalfAreaCount),
      finger_count_total: Int32(count.totalCount),
      palm_count_upper_half_area: Int32(count.upperHalfAreaPalmCount),
      palm_count_lower_half_area: Int32(count.lowerHalfAreaPalmCount),
      palm_count_left_half_area: Int32(count.leftHalfAreaPalmCount),
      palm_count_right_half_area: Int32(count.rightHalfAreaPalmCount),
      palm_count_total: Int32(count.totalPalmCount)))

  if requested {
    VariableUpdateLog.shared.append(count)
  }
}

@MainActor
final class MECoreServiceDaemonClient {
  static let shared = MECoreServiceDaemonClient()

  private var cancellables: Set<AnyCancellable> = []
  private var connectionChangedTask: Task<Void, Never>?

  init() {
    FingerManager.shared.$fingerCount
      .removeDuplicates()
      .sink { newValue in
        staticSetCoreServiceVariables(newValue)
      }
      .store(in: &cancellables)
  }

  func connectionChanged(_ connected: Bool) {
    connectionChangedTask?.cancel()

    connectionChangedTask = Task {
      if connected {
        do {
          // Sleep until devices are settled.
          try await Task.sleep(nanoseconds: NSEC_PER_SEC)
        } catch {
          return
        }
      }

      guard !Task.isCancelled else { return }

      if connected {
        MultitouchDeviceManager.shared.setCallback(true)
      } else {
        MultitouchDeviceManager.shared.setCallback(false)
      }

      staticSetCoreServiceVariables(FingerCount())
    }
  }
}
