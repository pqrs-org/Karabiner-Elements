import SwiftUI

@MainActor
public class EventObserver: ObservableObject {
  public static let shared = EventObserver()

  @Published var counter = 0

  // Prevent creating instances other than the shared singleton.
  private init() {}

  public func observed() -> Bool {
    game_pad_viewer_hid_value_monitor_observed()
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
        StickManager.shared.leftStick.horizontal.add(logicalMax, logicalMin, integerValue)
        StickManager.shared.leftStick.update()
      }
    }

    // usage::generic_desktop::y
    if usagePage == 0x1, usage == 0x31 {
      counter += 1
      Task { @MainActor in
        StickManager.shared.leftStick.vertical.add(logicalMax, logicalMin, integerValue)
        StickManager.shared.leftStick.update()
      }
    }

    //
    // Right Stick
    //

    // usage::generic_desktop::z
    if usagePage == 0x1, usage == 0x32 {
      counter += 1
      Task { @MainActor in
        StickManager.shared.rightStick.horizontal.add(logicalMax, logicalMin, integerValue)
        StickManager.shared.rightStick.update()
      }
    }

    // usage::generic_desktop::rz
    if usagePage == 0x1, usage == 0x35 {
      counter += 1
      Task { @MainActor in
        StickManager.shared.rightStick.vertical.add(logicalMax, logicalMin, integerValue)
        StickManager.shared.rightStick.update()
      }
    }
  }
}
