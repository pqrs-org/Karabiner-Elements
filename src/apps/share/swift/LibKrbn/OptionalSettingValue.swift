import Combine

extension LibKrbn {
  class OptionalSettingValue<T: Equatable>: ObservableObject, Equatable {
    private let setFunction: (_ newValue: T) -> Void
    private let unsetFunction: () -> Void
    private var didSetEnabled = false

    init(
      hasFunction: () -> Bool,
      getFunction: () -> T,
      setFunction: @escaping (_ newValue: T) -> Void,
      unsetFunction: @escaping () -> Void
    ) {
      self.setFunction = setFunction
      self.unsetFunction = unsetFunction

      overwrite = hasFunction()
      value = getFunction()

      didSetEnabled = true
    }

    @Published var overwrite: Bool = false {
      didSet {
        if didSetEnabled {
          save()
        }
      }
    }

    @Published var value: T {
      didSet {
        if didSetEnabled {
          save()
        }
      }
    }

    private func save() {
      if overwrite {
        setFunction(value)
      } else {
        unsetFunction()
      }

      Settings.shared.save()
    }

    public static func == (lhs: OptionalSettingValue, rhs: OptionalSettingValue) -> Bool {
      lhs.overwrite == rhs.overwrite && lhs.value == rhs.value
    }
  }
}
