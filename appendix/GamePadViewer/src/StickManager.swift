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

  struct HistoryEntry {
    let time = Date()
    let deltaMagnitude: Double
  }

  class Stick: ObservableObject {
    @Published var horizontal = StickSensor()
    @Published var vertical = StickSensor()
    @Published var radian = 0.0
    @Published var magnitude = 0.0
    @Published var deltaMagnitude = 0.0
    @Published var pointerX = 0.5  // 0.0 ... 1.0
    @Published var pointerY = 0.5  // 0.0 ... 1.0
    @Published var continuedMovementMagnitude = 0.0
    @Published var history: [HistoryEntry] = []
    var previousMagnitude = 0.0

    var continuedMovementTimer: Cancellable?
    var continuedMovementCalled = false

    @MainActor
    public func update() {
      let now = Date()

      radian = atan2(vertical.lastDoubleValue, horizontal.lastDoubleValue)
      magnitude = min(
        1.0,
        sqrt(pow(vertical.lastDoubleValue, 2) + pow(horizontal.lastDoubleValue, 2)))

      deltaMagnitude = max(0.0, magnitude - previousMagnitude)

      history.append(HistoryEntry(deltaMagnitude: deltaMagnitude))
      history.removeAll(where: { now.timeIntervalSince($0.time) > 0.1 })

      let continuedMovementThreshold = 1.0

      if magnitude >= continuedMovementThreshold {
        if continuedMovementTimer == nil {
          continuedMovementMagnitude =
            (history.max { a, b in a.deltaMagnitude < b.deltaMagnitude })?.deltaMagnitude ?? 0.0

          continuedMovementTimer = Timer.publish(
            every: 0.3,  // 300 ms
            on: .main, in: .default
          ).autoconnect()
            .sink { _ in
              self.updatePointerXY(magnitude: self.continuedMovementMagnitude, radian: self.radian)
              if !self.continuedMovementCalled {
                self.continuedMovementTimer = Timer.publish(
                  every: 0.02,  // 20 ms
                  on: .main, in: .default
                )
                .autoconnect().sink { _ in
                  self.updatePointerXY(
                    magnitude: self.continuedMovementMagnitude, radian: self.radian)
                }
                self.continuedMovementCalled = true
              }
            }

          continuedMovementCalled = false
        }
      } else {
        continuedMovementTimer = nil
      }

      let deltaMagnitudeThreshold = 0.01
      if 0 < deltaMagnitude {
        if deltaMagnitude < deltaMagnitudeThreshold {
          return
        }

        updatePointerXY(magnitude: deltaMagnitude, radian: radian)
      }

      previousMagnitude = magnitude
    }

    @MainActor
    private func updatePointerXY(magnitude: Double, radian: Double) {
      let scale = 1.0 / 16

      pointerX += magnitude * cos(radian) * scale
      pointerX = max(0.0, min(1.0, pointerX))
      pointerY -= magnitude * sin(radian) * scale
      pointerY = max(0.0, min(1.0, pointerY))
    }
  }

  @Published var rightStick = Stick()
}
