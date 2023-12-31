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
    @Published var radianDiff = 0.0
    @Published var deltaHorizontal = 0.0
    @Published var deltaVertical = 0.0
    @Published var deltaRadian = 0.0
    @Published var deltaMagnitude = 0.0
    @Published var pointerX = 0.5  // 0.0 ... 1.0
    @Published var pointerY = 0.5  // 0.0 ... 1.0
    @Published var continuedMovementMagnitude = 0.0
    let updateTimerInterval = 0.02  // 20 ms
    var previousHorizontalDoubleValue = 0.0
    var previousVerticalDoubleValue = 0.0
    var previousMagnitude = 0.0

    var updateTimer: Cancellable?

    @MainActor
    func setUpdateTimer() {
      if updateTimer == nil {
        updateTimer = Timer.publish(every: updateTimerInterval, on: .main, in: .default)
          .autoconnect().sink { _ in
            self.update()
          }
      }
    }

    @MainActor
    private func update() {
      deltaHorizontal = horizontal.lastDoubleValue - previousHorizontalDoubleValue
      deltaVertical = vertical.lastDoubleValue - previousVerticalDoubleValue
      deltaRadian = atan2(deltaVertical, deltaHorizontal)
      deltaMagnitude = min(1.0, sqrt(pow(deltaHorizontal, 2) + pow(deltaVertical, 2)))

      radian = atan2(vertical.lastDoubleValue, horizontal.lastDoubleValue)
      magnitude = min(
        1.0,
        sqrt(pow(vertical.lastDoubleValue, 2) + pow(horizontal.lastDoubleValue, 2)))

      let continuedMovementThreshold = 1.0
      let continuedMovementMinimumValue = 0.01

      if magnitude >= continuedMovementThreshold {
        if continuedMovementMagnitude == 0.0 {
          continuedMovementMagnitude = deltaMagnitude
        }
        continuedMovementMagnitude = max(continuedMovementMinimumValue, continuedMovementMagnitude)

        deltaRadian = radian
        deltaMagnitude = continuedMovementMagnitude
      } else {
        continuedMovementMagnitude = 0.0
      }

      let deltaMagnitudeThreshold = 0.01
      if deltaMagnitude < deltaMagnitudeThreshold {
        return
      }

      if magnitude >= previousMagnitude {
        let scale = 1.0 / 16

        pointerX += deltaMagnitude * cos(deltaRadian) * scale
        pointerX = max(0.0, min(1.0, pointerX))
        pointerY -= deltaMagnitude * sin(deltaRadian) * scale
        pointerY = max(0.0, min(1.0, pointerY))
      }

      if magnitude < 1.0 {
        updateTimer?.cancel()
        updateTimer = nil
      }

      previousHorizontalDoubleValue = horizontal.lastDoubleValue
      previousVerticalDoubleValue = vertical.lastDoubleValue
      previousMagnitude = magnitude
    }
  }

  static public func radianDifference(_ r1: Double, _ r2: Double) -> Double {
    let diff = abs(r1 - r2).truncatingRemainder(dividingBy: 2 * Double.pi)
    if diff > Double.pi {
      return 2 * Double.pi - diff
    }
    return diff
  }

  @Published var rightStick = Stick()
}
