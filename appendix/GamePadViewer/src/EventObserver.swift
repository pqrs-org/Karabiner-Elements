import SwiftUI

private func callback(
  _ deviceId: UInt64,
  _ usagePage: Int32,
  _ usage: Int32,
  _ logicalMax: Int64,
  _ logicalMin: Int64,
  _ integerValue: Int64
) {
  Task { @MainActor in
    EventObserver.shared.update(usagePage, usage, logicalMax, logicalMin, integerValue)
  }
}

public class EventObserver: ObservableObject {
  public static let shared = EventObserver()

  @ObservedObject var stickManager = StickManager.shared
  @Published var counter = 0

  // We register the callback in the `start` method rather than in `init`.
  // If libkrbn_register_*_callback is called within init, there is a risk that `init` could be invoked again from the callback through `shared` before the initial `init` completes.

  public func start() {
    libkrbn_enable_hid_value_monitor()

    libkrbn_register_hid_value_arrived_callback(callback)
  }

  public func stop() {
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
    // Left Stick
    //

    // usage::generic_desktop::x
    if usagePage == 0x1, usage == 0x30 {
      counter += 1
      Task { @MainActor in
        stickManager.leftStick.horizontal.add(logicalMax, logicalMin, integerValue)
        stickManager.leftStick.update()

        stickManager.converter.convert()
      }
    }

    // usage::generic_desktop::y
    if usagePage == 0x1, usage == 0x31 {
      counter += 1
      Task { @MainActor in
        stickManager.leftStick.vertical.add(logicalMax, logicalMin, integerValue)
        stickManager.leftStick.update()

        stickManager.converter.convert()
      }
    }

    //
    // Right Stick
    //

    // usage::generic_desktop::z
    if usagePage == 0x1, usage == 0x32 {
      counter += 1
      Task { @MainActor in
        stickManager.rightStick.horizontal.add(logicalMax, logicalMin, integerValue)
        stickManager.rightStick.update()

        stickManager.converter.convert()
      }
    }

    // usage::generic_desktop::rz
    if usagePage == 0x1, usage == 0x35 {
      counter += 1
      Task { @MainActor in
        stickManager.rightStick.vertical.add(logicalMax, logicalMin, integerValue)
        stickManager.rightStick.update()

        stickManager.converter.convert()
      }
    }
  }
}
