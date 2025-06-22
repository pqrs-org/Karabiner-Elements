import Combine
import SwiftUI

@MainActor
public class StickManager {
  public static let shared = StickManager()

  class StickSensor: ObservableObject {
    @Published var lastDoubleValue = 0.0

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

  class Stick: ObservableObject, Equatable {
    public let id = UUID()

    @Published var horizontal = StickSensor()
    @Published var vertical = StickSensor()

    @Published var radian = 0.0
    @Published var magnitude = 0.0

    public func update() {
      radian = atan2(vertical.lastDoubleValue, horizontal.lastDoubleValue)
      magnitude = min(
        1.0,
        sqrt(pow(vertical.lastDoubleValue, 2) + pow(horizontal.lastDoubleValue, 2)))
    }

    public static func == (lhs: Stick, rhs: Stick) -> Bool {
      lhs.id == rhs.id
    }
  }

  @Published var leftStick = Stick()
  @Published var rightStick = Stick()
}
