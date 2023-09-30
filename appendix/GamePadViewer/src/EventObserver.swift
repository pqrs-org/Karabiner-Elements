import SwiftUI

private func callback(
  _ deviceId: UInt64,
  _ usagePage: Int32,
  _ usage: Int32,
  _ logicalMax: Int64,
  _ logicalMin: Int64,
  _ integerValue: Int64,
  _ context: UnsafeMutableRawPointer?
) {
  Task { @MainActor in
    EventObserver.shared.update(usagePage, usage, logicalMax, logicalMin, integerValue)
  }
}

public class EventObserver: ObservableObject {
  class Stick: ObservableObject {
    @Published var value = 0.0
    @Published var arrivedAt = Date()
    @Published var acceleration = 0.0

    func update(
      _ logicalMax: Int64,
      _ logicalMin: Int64,
      _ integerValue: Int64
    ) {
      if logicalMax != logicalMin {
        let previousValue = value
        let previousArrivedAt = arrivedAt

        value = (Double(integerValue - logicalMin) / Double(logicalMax - logicalMin) - 0.5) * 2.0
        arrivedAt = Date()

        let interval = arrivedAt.timeIntervalSince(previousArrivedAt)
        if interval > 0 {
          acceleration = (value - previousValue) / interval
        } else {
          acceleration = 0
        }
      }
    }
  }

  public static let shared = EventObserver()

  @Published var counter = 0
  @ObservedObject var rightStickX = Stick()
  @ObservedObject var rightStickY = Stick()

  private init() {
    libkrbn_enable_hid_value_monitor(callback, nil)
  }

  deinit {
    libkrbn_disable_hid_value_monitor()
  }

  public func observed() -> Bool {
    libkrbn_hid_value_monitor_observed()
  }

  public func update(
    _ usagePage: Int32,
    _ usage: Int32,
    _ logicalMax: Int64,
    _ logicalMin: Int64,
    _ integerValue: Int64
  ) {
    //
    // Right Stick
    //

    // usage::generic_desktop::z
    if usagePage == 0x1, usage == 0x32 {
      counter += 1
      rightStickX.update(logicalMax, logicalMin, integerValue)
    }

    // usage::generic_desktop::rz
    if usagePage == 0x1, usage == 0x35 {
      counter += 1
      rightStickY.update(logicalMax, logicalMin, integerValue)
    }
  }
}
