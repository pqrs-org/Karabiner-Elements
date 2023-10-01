import Combine
import SwiftUI

public class StickManager: ObservableObject {
  public static let shared = StickManager()

  struct Event {
    var arrivedAt: Date
    var acceleration: Double
  }

  class Stick: ObservableObject {
    @Published var value = 0.0
    @Published var lastArrivedAt = Date()
    @Published var lastInterval = 0.0
    @Published var lastAcceleration = 0.0
    var eventHistories: [Event] = []
    var timer: AnyCancellable?

    init() {
      timer = Timer.publish(
        every: 0.02,  //  20 ms
        on: .main,
        in: .common
      )
      .autoconnect()
      .sink { [unowned self] _ in
        let attenuation = 0.01
        if abs(self.lastAcceleration) < attenuation {
          self.lastAcceleration = 0.0
        } else {
          if self.lastAcceleration > 0 {
            self.lastAcceleration -= attenuation
          } else {
            self.lastAcceleration += attenuation
          }
        }
      }
    }

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
        print("value \(value)")

        lastArrivedAt = now

        lastInterval = max(
          now.timeIntervalSince(previousArrivedAt),
          0.001  // 1 ms
        )

        let deadzone = 0.1
        if abs(value) < deadzone {
          lastAcceleration = 0.0
        } else {
          // -1000.0 ... 1000.0
          let acceleration = (value - previousValue) / lastInterval

          while eventHistories.count > 0 && now.timeIntervalSince(eventHistories[0].arrivedAt) > 1.0
          {
            eventHistories.removeFirst()
          }
          eventHistories.append(Event(arrivedAt: now, acceleration: acceleration))

          let max = eventHistories.max { a, b in abs(a.acceleration) < abs(b.acceleration) }
          lastAcceleration = max?.acceleration ?? 0.0
        }
      }
    }
  }

  @Published var rightStickX = Stick()
  @Published var rightStickY = Stick()
}
