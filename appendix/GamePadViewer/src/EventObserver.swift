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
  public static let shared = EventObserver()

  @ObservedObject var stickManager = StickManager.shared
  @Published var counter = 0

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
      Task { @MainActor in
        stickManager.rightStick.horizontal.add(logicalMax, logicalMin, integerValue)

        stickManager.rightStick.horizontal.update()
        stickManager.rightStick.vertical.update()
      }
    }

    // usage::generic_desktop::rz
    if usagePage == 0x1, usage == 0x35 {
      counter += 1
      Task { @MainActor in
        stickManager.rightStick.vertical.add(logicalMax, logicalMin, integerValue)

        stickManager.rightStick.horizontal.update()
        stickManager.rightStick.vertical.update()
      }
    }
  }
}
