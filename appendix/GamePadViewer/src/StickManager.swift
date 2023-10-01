import SwiftUI

public class StickManager: ObservableObject {
  public static let shared = StickManager()

  class Stick: ObservableObject {
    @Published var value = 0.0
    @Published var arrivedAt = Date()
    @Published var interval = 0.0
    @Published var acceleration = 0.0

    func update(
      _ logicalMax: Int64,
      _ logicalMin: Int64,
      _ integerValue: Int64
    ) {
      if logicalMax != logicalMin {
        let previousValue = value
        let previousArrivedAt = arrivedAt

        // -1.0 ... 1.0
        value = (Double(integerValue - logicalMin) / Double(logicalMax - logicalMin) - 0.5) * 2.0
        arrivedAt = Date()

        interval = max(
          arrivedAt.timeIntervalSince(previousArrivedAt),
          0.001  // 1 ms
        )

        let deadzone = 0.05
        if abs(value) < deadzone {
          acceleration = 0
          return
        }

        // -1000.0 ... 1000.0
        acceleration = (value - previousValue) / interval
      }
    }
  }

  @Published var rightStickX = Stick()
  @Published var rightStickY = Stick()
}
