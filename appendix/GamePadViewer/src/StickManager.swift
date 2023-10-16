import Combine
import SwiftUI

public class StickManager: ObservableObject {
  public static let shared = StickManager()

  class StickSensor: ObservableObject {
    @Published var lastDoubleValue = 0.0

    @MainActor
    func add(
      _ logicalMax: Int64,
      _ logicalMin: Int64,
      _ integerValue: Int64
    ) {
      if logicalMax != logicalMin {
        // -1.0 ... 1.0
        lastDoubleValue =
          (Double(integerValue - logicalMin) / Double(logicalMax - logicalMin) - 0.5) * 2.0
      }
    }
  }

  class Stick: ObservableObject {
    @Published var horizontal = StickSensor()
    @Published var vertical = StickSensor()
    @Published var radian = 0.0
    @Published var magnitude = 0.0
    @Published var deadzoneEnteredAt = Date()
    @Published var deadzoneLeftAt = Date()
    @Published var strokeAcceleration = 0.0
    let remainDeadzoneThresholdMilliseconds: UInt64 = 100
    let strokeAccelerationMeasurementTime = 0.05  // 50 ms

    var deadzoneTask: Task<(), Never>?

    @MainActor
    func update() {
      let now = Date()

      radian = atan2(vertical.lastDoubleValue, horizontal.lastDoubleValue)
      magnitude = min(
        1.0,
        sqrt(pow(vertical.lastDoubleValue, 2) + pow(horizontal.lastDoubleValue, 2)))

      let deadzone = 0.1
      if abs(vertical.lastDoubleValue) < deadzone && abs(horizontal.lastDoubleValue) < deadzone {
        if deadzoneTask == nil {
          deadzoneTask = Task { @MainActor in
            do {
              try await Task.sleep(nanoseconds: remainDeadzoneThresholdMilliseconds * NSEC_PER_MSEC)

              deadzoneEnteredAt = now

              strokeAcceleration = 0.0
            } catch {
              print("cancelled")
            }
          }
        }
      } else {
        deadzoneTask?.cancel()
        deadzoneTask = nil

        if deadzoneEnteredAt > deadzoneLeftAt {
          deadzoneLeftAt = now
        }
      }

      if now.timeIntervalSince(deadzoneLeftAt) < strokeAccelerationMeasurementTime {
        if strokeAcceleration < magnitude {
          strokeAcceleration = magnitude
        }
      }
    }
  }

  @Published var rightStick = Stick()
}
