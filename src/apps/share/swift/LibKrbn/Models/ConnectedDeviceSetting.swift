import Foundation

extension LibKrbn {
  final class ConnectedDeviceSetting: Identifiable, Equatable, ObservableObject {
    private var didSetEnabled = false

    var id = UUID()
    var connectedDevice: ConnectedDevice

    init(_ connectedDevice: ConnectedDevice) {
      self.connectedDevice = connectedDevice

      let ignore = libkrbn_core_configuration_get_selected_profile_device_ignore(
        connectedDevice.libkrbnDeviceIdentifiers)
      modifyEvents = !ignore

      self.manipulateCapsLockLed =
        libkrbn_core_configuration_get_selected_profile_device_manipulate_caps_lock_led(
          connectedDevice.libkrbnDeviceIdentifiers)

      self.treatAsBuiltInKeyboard =
        libkrbn_core_configuration_get_selected_profile_device_treat_as_built_in_keyboard(
          connectedDevice.libkrbnDeviceIdentifiers)

      self.disableBuiltInKeyboardIfExists =
        libkrbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(
          connectedDevice.libkrbnDeviceIdentifiers)

      self.mouseFlipX =
        libkrbn_core_configuration_get_selected_profile_device_mouse_flip_x(
          connectedDevice.libkrbnDeviceIdentifiers)

      self.mouseFlipY =
        libkrbn_core_configuration_get_selected_profile_device_mouse_flip_y(
          connectedDevice.libkrbnDeviceIdentifiers)

      self.mouseFlipVerticalWheel =
        libkrbn_core_configuration_get_selected_profile_device_mouse_flip_vertical_wheel(
          connectedDevice.libkrbnDeviceIdentifiers)

      self.mouseFlipHorizontalWheel =
        libkrbn_core_configuration_get_selected_profile_device_mouse_flip_horizontal_wheel(
          connectedDevice.libkrbnDeviceIdentifiers)

      self.mouseSwapXY =
        libkrbn_core_configuration_get_selected_profile_device_mouse_swap_xy(
          connectedDevice.libkrbnDeviceIdentifiers)

      self.mouseSwapWheels =
        libkrbn_core_configuration_get_selected_profile_device_mouse_swap_wheels(
          connectedDevice.libkrbnDeviceIdentifiers)

      self.gamePadSwapSticks =
        libkrbn_core_configuration_get_selected_profile_device_game_pad_swap_sticks(
          connectedDevice.libkrbnDeviceIdentifiers)

      gamePadXYStickContinuedMovementAbsoluteMagnitudeThreshold = OptionalSettingValue<Double>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(
              connectedDevice.libkrbnDeviceIdentifiers)
        },
        getFunction: {
          return
            libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(
              connectedDevice.libkrbnDeviceIdentifiers)
        },
        setFunction: { (_ newValue: Double) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(
            connectedDevice.libkrbnDeviceIdentifiers, newValue)
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(
            connectedDevice.libkrbnDeviceIdentifiers)
        }
      )

      gamePadXYStickContinuedMovementIntervalMilliseconds = OptionalSettingValue<Int>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(
              connectedDevice.libkrbnDeviceIdentifiers)
        },
        getFunction: {
          return Int(
            libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(
              connectedDevice.libkrbnDeviceIdentifiers))
        },
        setFunction: { (_ newValue: Int) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(
            connectedDevice.libkrbnDeviceIdentifiers, Int32(newValue))
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(
            connectedDevice.libkrbnDeviceIdentifiers)
        }
      )

      gamePadXYStickFlickingInputWindowMilliseconds = OptionalSettingValue<Int>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_xy_stick_flicking_input_window_milliseconds(
              connectedDevice.libkrbnDeviceIdentifiers)
        },
        getFunction: {
          return Int(
            libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_flicking_input_window_milliseconds(
              connectedDevice.libkrbnDeviceIdentifiers))
        },
        setFunction: { (_ newValue: Int) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_flicking_input_window_milliseconds(
            connectedDevice.libkrbnDeviceIdentifiers, Int32(newValue))
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_xy_stick_flicking_input_window_milliseconds(
            connectedDevice.libkrbnDeviceIdentifiers)
        }
      )

      gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold = OptionalSettingValue<Double>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(
              connectedDevice.libkrbnDeviceIdentifiers
            )
        },
        getFunction: {
          return
            libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(
              connectedDevice.libkrbnDeviceIdentifiers)
        },
        setFunction: { (_ newValue: Double) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(
            connectedDevice.libkrbnDeviceIdentifiers, newValue)
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(
            connectedDevice.libkrbnDeviceIdentifiers)
        }
      )

      gamePadWheelsStickContinuedMovementIntervalMilliseconds = OptionalSettingValue<Int>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(
              connectedDevice.libkrbnDeviceIdentifiers)
        },
        getFunction: {
          return Int(
            libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(
              connectedDevice.libkrbnDeviceIdentifiers)
          )
        },
        setFunction: { (_ newValue: Int) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(
            connectedDevice.libkrbnDeviceIdentifiers, Int32(newValue))
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(
            connectedDevice.libkrbnDeviceIdentifiers)
        }
      )

      gamePadWheelsStickFlickingInputWindowMilliseconds = OptionalSettingValue<Int>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_wheels_stick_flicking_input_window_milliseconds(
              connectedDevice.libkrbnDeviceIdentifiers)
        },
        getFunction: {
          return Int(
            libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_flicking_input_window_milliseconds(
              connectedDevice.libkrbnDeviceIdentifiers))
        },
        setFunction: { (_ newValue: Int) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_flicking_input_window_milliseconds(
            connectedDevice.libkrbnDeviceIdentifiers, Int32(newValue))
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_wheels_stick_flicking_input_window_milliseconds(
            connectedDevice.libkrbnDeviceIdentifiers)
        }
      )

      gamePadStickXFormula = OptionalSettingValue<String>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_stick_x_formula(
              connectedDevice.libkrbnDeviceIdentifiers)
        },
        getFunction: {
          var buffer = [Int8](repeating: 0, count: 16384)
          libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_x_formula(
            connectedDevice.libkrbnDeviceIdentifiers, &buffer, buffer.count)

          return String(cString: buffer).trimmingCharacters(in: .whitespacesAndNewlines)
        },
        setFunction: { (_ newValue: String) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_x_formula(
            connectedDevice.libkrbnDeviceIdentifiers, newValue.cString(using: .utf8))
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_stick_x_formula(
            connectedDevice.libkrbnDeviceIdentifiers)
        }
      )

      gamePadStickYFormula = OptionalSettingValue<String>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_stick_y_formula(
              connectedDevice.libkrbnDeviceIdentifiers)
        },
        getFunction: {
          var buffer = [Int8](repeating: 0, count: 16384)
          libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_y_formula(
            connectedDevice.libkrbnDeviceIdentifiers, &buffer, buffer.count)

          return String(cString: buffer).trimmingCharacters(in: .whitespacesAndNewlines)
        },
        setFunction: { (_ newValue: String) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_y_formula(
            connectedDevice.libkrbnDeviceIdentifiers, newValue.cString(using: .utf8)
          )
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_stick_y_formula(
            connectedDevice.libkrbnDeviceIdentifiers)
        }
      )

      gamePadStickVerticalWheelFormula = OptionalSettingValue<String>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_stick_vertical_wheel_formula(
              connectedDevice.libkrbnDeviceIdentifiers)
        },
        getFunction: {
          var buffer = [Int8](repeating: 0, count: 16384)
          libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_vertical_wheel_formula(
            connectedDevice.libkrbnDeviceIdentifiers, &buffer, buffer.count)

          return String(cString: buffer).trimmingCharacters(in: .whitespacesAndNewlines)
        },
        setFunction: { (_ newValue: String) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_vertical_wheel_formula(
            connectedDevice.libkrbnDeviceIdentifiers, newValue.cString(using: .utf8))
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_stick_vertical_wheel_formula(
            connectedDevice.libkrbnDeviceIdentifiers)
        }
      )

      gamePadStickHorizontalWheelFormula = OptionalSettingValue<String>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_stick_horizontal_wheel_formula(
              connectedDevice.libkrbnDeviceIdentifiers)
        },
        getFunction: {
          var buffer = [Int8](repeating: 0, count: 16384)
          libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_horizontal_wheel_formula(
            connectedDevice.libkrbnDeviceIdentifiers, &buffer, buffer.count)

          return String(cString: buffer).trimmingCharacters(in: .whitespacesAndNewlines)
        },
        setFunction: { (_ newValue: String) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_horizontal_wheel_formula(
            connectedDevice.libkrbnDeviceIdentifiers, newValue.cString(using: .utf8))
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_stick_horizontal_wheel_formula(
            connectedDevice.libkrbnDeviceIdentifiers)
        }
      )

      simpleModifications = LibKrbn.Settings.shared.makeSimpleModifications(connectedDevice)
      fnFunctionKeys = LibKrbn.Settings.shared.makeFnFunctionKeys(connectedDevice)

      didSetEnabled = true
    }

    @Published var modifyEvents: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_ignore(
            connectedDevice.libkrbnDeviceIdentifiers, !modifyEvents)
          Settings.shared.save()
        }
      }
    }

    @Published var manipulateCapsLockLed: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_manipulate_caps_lock_led(
            connectedDevice.libkrbnDeviceIdentifiers, manipulateCapsLockLed)
          Settings.shared.save()
        }
      }
    }

    @Published var treatAsBuiltInKeyboard: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_treat_as_built_in_keyboard(
            connectedDevice.libkrbnDeviceIdentifiers, treatAsBuiltInKeyboard)
          Settings.shared.save()
        }
      }
    }

    @Published var disableBuiltInKeyboardIfExists: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(
            connectedDevice.libkrbnDeviceIdentifiers, disableBuiltInKeyboardIfExists)
          Settings.shared.save()
        }
      }
    }

    @Published var mouseFlipX: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_mouse_flip_x(
            connectedDevice.libkrbnDeviceIdentifiers, mouseFlipX)
          Settings.shared.save()
        }
      }
    }

    @Published var mouseFlipY: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_mouse_flip_y(
            connectedDevice.libkrbnDeviceIdentifiers, mouseFlipY)
          Settings.shared.save()
        }
      }
    }

    @Published var mouseFlipVerticalWheel: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_mouse_flip_vertical_wheel(
            connectedDevice.libkrbnDeviceIdentifiers, mouseFlipVerticalWheel)
          Settings.shared.save()
        }
      }
    }
    @Published var mouseFlipHorizontalWheel: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_mouse_flip_horizontal_wheel(
            connectedDevice.libkrbnDeviceIdentifiers, mouseFlipHorizontalWheel)
          Settings.shared.save()
        }
      }
    }

    @Published var mouseSwapXY: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_mouse_swap_xy(
            connectedDevice.libkrbnDeviceIdentifiers, mouseSwapXY)
          Settings.shared.save()
        }
      }
    }

    @Published var mouseSwapWheels: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_mouse_swap_wheels(
            connectedDevice.libkrbnDeviceIdentifiers, mouseSwapWheels)
          Settings.shared.save()
        }
      }
    }

    @Published var gamePadSwapSticks: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_game_pad_swap_sticks(
            connectedDevice.libkrbnDeviceIdentifiers, gamePadSwapSticks)
          Settings.shared.save()
        }
      }
    }

    var gamePadXYStickContinuedMovementAbsoluteMagnitudeThreshold: OptionalSettingValue<Double>
    var gamePadXYStickContinuedMovementIntervalMilliseconds: OptionalSettingValue<Int>
    var gamePadXYStickFlickingInputWindowMilliseconds: OptionalSettingValue<Int>
    var gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold: OptionalSettingValue<Double>
    var gamePadWheelsStickContinuedMovementIntervalMilliseconds: OptionalSettingValue<Int>
    var gamePadWheelsStickFlickingInputWindowMilliseconds: OptionalSettingValue<Int>
    var gamePadStickXFormula: OptionalSettingValue<String>
    var gamePadStickYFormula: OptionalSettingValue<String>
    var gamePadStickVerticalWheelFormula: OptionalSettingValue<String>
    var gamePadStickHorizontalWheelFormula: OptionalSettingValue<String>

    @Published var simpleModifications: [SimpleModification] = []
    @Published var fnFunctionKeys: [SimpleModification] = []

    public static func == (lhs: ConnectedDeviceSetting, rhs: ConnectedDeviceSetting) -> Bool {
      lhs.id == rhs.id
    }
  }
}
