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
    let horizontalStickSensorValue: Double
    let verticalStickSensorValue: Double
  }

  class Stick: ObservableObject, Equatable {
    public let id = UUID()

    @Published var horizontal = StickSensor()
    @Published var vertical = StickSensor()
    @Published var history: [HistoryEntry] = []

    @Published var radian = 0.0
    @Published var magnitude = 0.0
    @Published var deltaMagnitude = 0.0

    var previousMagnitude = 0.0

    @MainActor
    public func update() {
      let now = Date()

      radian = atan2(vertical.lastDoubleValue, horizontal.lastDoubleValue)
      magnitude = min(
        1.0,
        sqrt(pow(vertical.lastDoubleValue, 2) + pow(horizontal.lastDoubleValue, 2)))

      deltaMagnitude = max(0.0, magnitude - previousMagnitude)

      history.append(
        HistoryEntry(
          horizontalStickSensorValue: horizontal.lastDoubleValue,
          verticalStickSensorValue: vertical.lastDoubleValue))
      history.removeAll(where: { now.timeIntervalSince($0.time) > 0.1 })
    }

    @MainActor
    public func getMaxDistanceInHistory() -> Double {
      var distance = 0.0

      history.forEach { h1 in
        history.forEach { h2 in
          let h = h1.horizontalStickSensorValue - h2.horizontalStickSensorValue
          let v = h1.verticalStickSensorValue - h2.verticalStickSensorValue
          let d = sqrt(h * h + v * v)
          if d > distance {
            distance = d
          }
        }
      }

      return min(1.0, distance)
    }

    @MainActor func updatePreviousMagnitude() {
      previousMagnitude = magnitude
    }

    public static func == (lhs: Stick, rhs: Stick) -> Bool {
      lhs.id == rhs.id
    }
  }

  class Converter: ObservableObject {
    @Published var pointerX = 0.5  // 0.0 ... 1.0
    @Published var pointerY = 0.5  // 0.0 ... 1.0
    @Published var continuedMovementMagnitude = 0.0

    let continuedMovementThreshold = 1.0
    let deltaMagnitudeThreshold = 0.01

    var continuedMovementPrimaryStick: Stick?
    var continuedMovementSecondaryStick: Stick?
    var continuedMovementTimer: Cancellable?
    var continuedMovementCalled = false

    @MainActor
    public func convert() {

      if let s = continuedMovementPrimaryStick {
        if s.magnitude < continuedMovementThreshold {
          continuedMovementPrimaryStick = nil
        }
      }

      if continuedMovementPrimaryStick == nil {
        if StickManager.shared.leftStick.magnitude >= continuedMovementThreshold {
          continuedMovementPrimaryStick = StickManager.shared.leftStick
        } else if StickManager.shared.rightStick.magnitude >= continuedMovementThreshold {
          continuedMovementPrimaryStick = StickManager.shared.rightStick
        }
      }

      if let continuedMovementPrimaryStick = continuedMovementPrimaryStick {
        continuedMovementSecondaryStick =
          continuedMovementPrimaryStick == StickManager.shared.leftStick
          ? StickManager.shared.rightStick
          : StickManager.shared.leftStick

        if continuedMovementTimer == nil {
          continuedMovementMagnitude = continuedMovementPrimaryStick.getMaxDistanceInHistory()

          continuedMovementTimer = Timer.publish(
            every: 0.3,  // 300 ms
            on: .main, in: .default
          ).autoconnect()
            .sink { _ in
              self.updatePointerXY(
                magnitude: self.getAdjustedContinuedMovementMagnitude(),
                radian: self.getAdjustedContinuedMovementRadian())

              if !self.continuedMovementCalled {
                self.continuedMovementTimer = Timer.publish(
                  every: 0.02,  // 20 ms
                  on: .main, in: .default
                )
                .autoconnect().sink { _ in

                  self.updatePointerXY(
                    magnitude: self.getAdjustedContinuedMovementMagnitude(),
                    radian: self.getAdjustedContinuedMovementRadian())
                }
                self.continuedMovementCalled = true
              }
            }

          continuedMovementCalled = false
        }
      } else {
        continuedMovementSecondaryStick = nil
        continuedMovementTimer = nil

        var keepPreviousMagnitude = false
        if 0 < StickManager.shared.rightStick.deltaMagnitude {
          if StickManager.shared.rightStick.deltaMagnitude < deltaMagnitudeThreshold {
            keepPreviousMagnitude = true
          } else {
            updatePointerXY(
              magnitude: StickManager.shared.rightStick.deltaMagnitude,
              radian: StickManager.shared.rightStick.radian)
          }
        }

        if !keepPreviousMagnitude {
          StickManager.shared.rightStick.updatePreviousMagnitude()
        }
      }
    }

    @MainActor
    private func getAdjustedContinuedMovementMagnitude() -> Double {
      let m2 = continuedMovementSecondaryStick?.magnitude ?? 0.0

      if m2 < deltaMagnitudeThreshold {
        return continuedMovementMagnitude
      } else {
        return continuedMovementMagnitude + m2
      }
    }

    @MainActor
    private func getAdjustedContinuedMovementRadian() -> Double {
      let r1 = continuedMovementPrimaryStick?.radian ?? 0.0
      let m2 = continuedMovementSecondaryStick?.magnitude ?? 0.0
      let r2 = continuedMovementSecondaryStick?.radian ?? 0.0

      if m2 < deltaMagnitudeThreshold {
        return r1
      } else {
        return (r1 + r2) / 2
      }
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

  @Published var leftStick = Stick()
  @Published var rightStick = Stick()
  @Published var converter = Converter()
}
