import Combine
import SwiftUI

public class StickManager: ObservableObject {
  public static let shared = StickManager()

  struct History {
    let timeStamp: Date
    let radian: Double
    var magnitude: Double
  }

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
    @Published var startingStroke = false
    @Published var deadzoneEnteredAt = Date()
    @Published var deadzoneLeftAt = Date()
    @Published var holdingAcceleration = 0.0
    @Published var holdingMagnitude = 0.0
    var histories: [History] = []
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
        deadzoneTask = Task { @MainActor in
          do {
            try await Task.sleep(nanoseconds: 200 * NSEC_PER_MSEC)

            if let last = histories.last {
              if last.timeStamp == now {
                deadzoneEnteredAt = now
                startingStroke = false

                histories.removeAll()
                holdingAcceleration = 0.0
                holdingMagnitude = 0.0
              }
            }
          } catch {
            print("cancelled")
          }
        }
      } else {
        deadzoneTask?.cancel()

        if deadzoneEnteredAt > deadzoneLeftAt {
          deadzoneLeftAt = now
        }

        if now.timeIntervalSince(deadzoneLeftAt) > strokeAccelerationMeasurementTime {
          startingStroke = false
        } else {
          startingStroke = true
        }
      }

      histories.removeAll(where: {
        if $0.timeStamp < deadzoneLeftAt {
          return true
        }

        return now.timeIntervalSince($0.timeStamp) > strokeAccelerationMeasurementTime
      })

      histories.append(
        History(
          timeStamp: now,
          radian: radian,
          magnitude: magnitude))

      if startingStroke {
        let minMagnitude = histories.min(by: { $0.magnitude < $1.magnitude })
        let maxMagnitude = histories.max(by: { $0.magnitude < $1.magnitude })
        let a = (maxMagnitude?.magnitude ?? 0) - (minMagnitude?.magnitude ?? 0)

        if holdingAcceleration < a {
          // Increase acceleration if magnitude is increased.
          if magnitude > holdingMagnitude {
            holdingAcceleration = a
          }
        } else {
          // Decrease acceleration if magnitude is decreased.
          if magnitude < holdingMagnitude - 0.1 {
            holdingAcceleration = a
          }
        }

        holdingMagnitude = magnitude
      }
    }
  }

  @Published var rightStick = Stick()
}
