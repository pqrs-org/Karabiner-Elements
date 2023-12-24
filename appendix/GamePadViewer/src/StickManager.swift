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
    @Published var strokeAccelerationDestinationValue = 0.0
    @Published var strokeAccelerationTransitionValue = 0.0
    @Published var strokeAccelerationTransitionMagnitude = 0.0
    @Published var deadzoneRadian = 0.0
    @Published var deadzoneMagnitude = 0.0
    @Published var accelerationFixed = false
    @Published var radianDiff = 0.0
    @Published var deltaHorizontal = 0.0
    @Published var deltaVertical = 0.0
    @Published var deltaRadian = 0.0
    @Published var deltaMagnitude = 0.0
    @Published var pointerX = 0.5  // 0.0 ... 1.0
    @Published var pointerY = 0.5  // 0.0 ... 1.0
    let remainDeadzoneThresholdMilliseconds: UInt64 = 100
    let strokeAccelerationMeasurementTime = 0.05  // 50 ms
    let updateTimerInterval = 0.02  // 20 ms
    var previousHorizontalDoubleValue = 0.0
    var previousVerticalDoubleValue = 0.0

    var deadzoneTask: Task<(), Never>?
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

      let deadzone = 0.1
      if abs(vertical.lastDoubleValue) < deadzone && abs(horizontal.lastDoubleValue) < deadzone {
        deltaMagnitude = 0.0

        if deadzoneTask == nil {
          deadzoneTask = Task { @MainActor in
            do {
              try await Task.sleep(nanoseconds: remainDeadzoneThresholdMilliseconds * NSEC_PER_MSEC)

              strokeAccelerationDestinationValue = 0.0
              strokeAccelerationTransitionValue = 0.0
              strokeAccelerationTransitionMagnitude = 0.0
              accelerationFixed = false

              updateTimer?.cancel()
              updateTimer = nil
            } catch {
              print("cancelled")
            }
          }
        }
      } else {
        if deadzoneTask != nil {
          deadzoneRadian = deltaRadian
          deadzoneMagnitude = magnitude

          deadzoneTask?.cancel()
          deadzoneTask = nil
        }
      }

      if deltaMagnitude > 0 {
        radianDiff = abs(deltaRadian - radian).truncatingRemainder(
          dividingBy: 2 * Double.pi)
        if radianDiff > Double.pi {
          radianDiff = 2 * Double.pi - radianDiff
        }

        if !accelerationFixed {
          let threshold = 0.174533  // 10 degree
          if radianDiff < threshold {
            strokeAccelerationDestinationValue += deltaMagnitude
            if strokeAccelerationDestinationValue > 1.0 {
              strokeAccelerationDestinationValue = 1.0
            }
            strokeAccelerationTransitionMagnitude =
              abs(strokeAccelerationDestinationValue - strokeAccelerationTransitionValue)
              / (1.0 / updateTimerInterval)
          } else if radianDiff > Double.pi - threshold && radianDiff < Double.pi + threshold {
            strokeAccelerationDestinationValue -= deltaMagnitude
            if strokeAccelerationDestinationValue < 0.0 {
              strokeAccelerationDestinationValue = 0.0
            }
            strokeAccelerationTransitionMagnitude =
              abs(strokeAccelerationDestinationValue - strokeAccelerationTransitionValue)
              / (1.0 / updateTimerInterval)
          } else {
            // accelerationFixed = true
          }
        }
      }

      if strokeAccelerationTransitionValue != strokeAccelerationDestinationValue {
        if strokeAccelerationTransitionValue < strokeAccelerationDestinationValue {
          strokeAccelerationTransitionValue += strokeAccelerationTransitionMagnitude
        } else if strokeAccelerationTransitionValue > strokeAccelerationDestinationValue {
          strokeAccelerationTransitionValue -= strokeAccelerationTransitionMagnitude
        }

        if abs(strokeAccelerationTransitionValue - strokeAccelerationDestinationValue)
          < strokeAccelerationTransitionMagnitude
        {
          strokeAccelerationTransitionValue = strokeAccelerationDestinationValue
        }
      }

      pointerX += deltaMagnitude * cos(deltaRadian)
      pointerX = max(0.0, min(1.0, pointerX))
      pointerY += deltaMagnitude * sin(deltaRadian)
      pointerY = max(0.0, min(1.0, pointerY))

      previousHorizontalDoubleValue = horizontal.lastDoubleValue
      previousVerticalDoubleValue = vertical.lastDoubleValue
    }
  }

  @Published var rightStick = Stick()
}
