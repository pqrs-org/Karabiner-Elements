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
    @Published var holdingAcceleration = 0.0
    @Published var holdingMagnitude = 0.0
    var histories: [History] = []

    @MainActor
    func update() {
      radian = atan2(vertical.lastDoubleValue, horizontal.lastDoubleValue)
      magnitude = min(
        1.0,
        sqrt(pow(vertical.lastDoubleValue, 2) + pow(horizontal.lastDoubleValue, 2)))

      let deadzone = 0.1
      if abs(vertical.lastDoubleValue) < deadzone && abs(horizontal.lastDoubleValue) < deadzone {
        histories.removeAll()
        holdingAcceleration = 0.0
        holdingMagnitude = 0.0
        return
      }

      let now = Date()

      histories.removeAll(where: {
        return now.timeIntervalSince($0.timeStamp) > 0.02  // 20 ms
      })

      histories.append(
        History(
          timeStamp: now,
          radian: radian,
          magnitude: magnitude))

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
        if magnitude < holdingMagnitude {
          holdingAcceleration = a
        }
      }

      holdingMagnitude = magnitude
    }
  }

  @Published var rightStick = Stick()
}
