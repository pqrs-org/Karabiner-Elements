import Combine
import SwiftUI

public class StickManager: ObservableObject {
  public static let shared = StickManager()

  struct Event {
    var arrivedAt: Date
    var acceleration: Double

    func attenuatedAcceleration(_ now: Date) -> Double {
      let attenuation = 5 * now.timeIntervalSince(arrivedAt)
      if abs(acceleration) < attenuation {
        return 0
      }
      if acceleration > 0 {
        return acceleration - attenuation
      } else {
        return acceleration + attenuation
      }
    }
  }

  class Stick: ObservableObject {
    @Published var value = 0.0
    @Published var lastArrivedAt = Date()
    @Published var lastInterval = 0.0
    @Published var lastAcceleration = 0.0
    var eventHistories: [Event] = []

    @MainActor
    func update(
      _ logicalMax: Int64,
      _ logicalMin: Int64,
      _ integerValue: Int64
    ) {
      if logicalMax != logicalMin {
        let previousValue = value
        let previousArrivedAt = lastArrivedAt

        let now = Date()

        // -1.0 ... 1.0
        value = (Double(integerValue - logicalMin) / Double(logicalMax - logicalMin) - 0.5) * 2.0

        lastArrivedAt = now

        lastInterval = max(
          now.timeIntervalSince(previousArrivedAt),
          0.001  // 1 ms
        )

        let deadzone = 0.1
        if abs(value) < deadzone {
          eventHistories.removeAll()
          lastAcceleration = 0.0
        } else {
          // -1000.0 ... 1000.0
          let acceleration = (value - previousValue) / lastInterval

          eventHistories.append(Event(arrivedAt: now, acceleration: acceleration))

          var sum = 0.0
          eventHistories.forEach { e in
            sum += e.attenuatedAcceleration(now)
          }
          lastAcceleration = sum

          eventHistories.removeAll(where: {
            let attenuatedAcceleration = $0.attenuatedAcceleration(now)
            return attenuatedAcceleration == 0 || sum * attenuatedAcceleration < 0
          })
        }
      }
    }
  }

  @Published var rightStickX = Stick()
  @Published var rightStickY = Stick()
}
