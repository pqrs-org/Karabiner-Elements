import Foundation

extension LibKrbn {
  final class ConnectedDeviceSetting: Identifiable, Equatable, ObservableObject {
    private var didSetEnabled = false

    var id = UUID()
    var connectedDevice: ConnectedDevice

    init(_ connectedDevice: ConnectedDevice) {
      self.connectedDevice = connectedDevice

      let ignore = libkrbn_core_configuration_get_selected_profile_device_ignore(
        Settings.shared.libkrbnCoreConfiguration,
        connectedDevice.libkrbnDeviceIdentifiers)
      modifyEvents = !ignore

      self.manipulateCapsLockLed =
        libkrbn_core_configuration_get_selected_profile_device_manipulate_caps_lock_led(
          Settings.shared.libkrbnCoreConfiguration,
          connectedDevice.libkrbnDeviceIdentifiers)

      self.treatAsBuiltInKeyboard =
        libkrbn_core_configuration_get_selected_profile_device_treat_as_built_in_keyboard(
          Settings.shared.libkrbnCoreConfiguration,
          connectedDevice.libkrbnDeviceIdentifiers)

      self.disableBuiltInKeyboardIfExists =
        libkrbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(
          Settings.shared.libkrbnCoreConfiguration,
          connectedDevice.libkrbnDeviceIdentifiers)

      self.mouseFlipX =
        libkrbn_core_configuration_get_selected_profile_device_mouse_flip_x(
          Settings.shared.libkrbnCoreConfiguration,
          connectedDevice.libkrbnDeviceIdentifiers)

      self.mouseFlipY =
        libkrbn_core_configuration_get_selected_profile_device_mouse_flip_y(
          Settings.shared.libkrbnCoreConfiguration,
          connectedDevice.libkrbnDeviceIdentifiers)

      self.mouseFlipVerticalWheel =
        libkrbn_core_configuration_get_selected_profile_device_mouse_flip_vertical_wheel(
          Settings.shared.libkrbnCoreConfiguration,
          connectedDevice.libkrbnDeviceIdentifiers)

      self.mouseFlipHorizontalWheel =
        libkrbn_core_configuration_get_selected_profile_device_mouse_flip_horizontal_wheel(
          Settings.shared.libkrbnCoreConfiguration,
          connectedDevice.libkrbnDeviceIdentifiers)

      self.mouseSwapXY =
        libkrbn_core_configuration_get_selected_profile_device_mouse_swap_xy(
          Settings.shared.libkrbnCoreConfiguration,
          connectedDevice.libkrbnDeviceIdentifiers)

      self.mouseSwapWheels =
        libkrbn_core_configuration_get_selected_profile_device_mouse_swap_wheels(
          Settings.shared.libkrbnCoreConfiguration,
          connectedDevice.libkrbnDeviceIdentifiers)

      self.gamePadSwapSticks =
        libkrbn_core_configuration_get_selected_profile_device_game_pad_swap_sticks(
          Settings.shared.libkrbnCoreConfiguration,
          connectedDevice.libkrbnDeviceIdentifiers)

      gamePadXYStickContinuedMovementAbsoluteMagnitudeThreshold = OptionalSettingValue<Double>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(
              Settings.shared.libkrbnCoreConfiguration,
              connectedDevice.libkrbnDeviceIdentifiers
            )
        },
        getFunction: {
          return
            libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(
              Settings.shared.libkrbnCoreConfiguration,
              connectedDevice.libkrbnDeviceIdentifiers
            )
        },
        setFunction: { (_ newValue: Double) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            newValue
          )
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers
          )
        }
      )

      gamePadXYStickContinuedMovementIntervalMilliseconds = OptionalSettingValue<Int>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(
              Settings.shared.libkrbnCoreConfiguration,
              connectedDevice.libkrbnDeviceIdentifiers
            )
        },
        getFunction: {
          return Int(
            libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(
              Settings.shared.libkrbnCoreConfiguration,
              connectedDevice.libkrbnDeviceIdentifiers
            ))
        },
        setFunction: { (_ newValue: Int) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            Int32(newValue)
          )
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers
          )
        }
      )

      gamePadXYStickFlickingInputWindowMilliseconds = OptionalSettingValue<Int>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_xy_stick_flicking_input_window_milliseconds(
              Settings.shared.libkrbnCoreConfiguration,
              connectedDevice.libkrbnDeviceIdentifiers
            )
        },
        getFunction: {
          return Int(
            libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_flicking_input_window_milliseconds(
              Settings.shared.libkrbnCoreConfiguration,
              connectedDevice.libkrbnDeviceIdentifiers
            ))
        },
        setFunction: { (_ newValue: Int) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_flicking_input_window_milliseconds(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            Int32(newValue)
          )
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_xy_stick_flicking_input_window_milliseconds(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers
          )
        }
      )

      gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold = OptionalSettingValue<Double>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(
              Settings.shared.libkrbnCoreConfiguration,
              connectedDevice.libkrbnDeviceIdentifiers
            )
        },
        getFunction: {
          return
            libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(
              Settings.shared.libkrbnCoreConfiguration,
              connectedDevice.libkrbnDeviceIdentifiers
            )
        },
        setFunction: { (_ newValue: Double) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            newValue
          )
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers
          )
        }
      )

      gamePadWheelsStickContinuedMovementIntervalMilliseconds = OptionalSettingValue<Int>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(
              Settings.shared.libkrbnCoreConfiguration,
              connectedDevice.libkrbnDeviceIdentifiers
            )
        },
        getFunction: {
          return Int(
            libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(
              Settings.shared.libkrbnCoreConfiguration,
              connectedDevice.libkrbnDeviceIdentifiers
            )
          )
        },
        setFunction: { (_ newValue: Int) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            Int32(newValue)
          )
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers
          )
        }
      )

      gamePadWheelsStickFlickingInputWindowMilliseconds = OptionalSettingValue<Int>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_wheels_stick_flicking_input_window_milliseconds(
              Settings.shared.libkrbnCoreConfiguration,
              connectedDevice.libkrbnDeviceIdentifiers
            )
        },
        getFunction: {
          return Int(
            libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_flicking_input_window_milliseconds(
              Settings.shared.libkrbnCoreConfiguration,
              connectedDevice.libkrbnDeviceIdentifiers
            ))
        },
        setFunction: { (_ newValue: Int) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_flicking_input_window_milliseconds(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            Int32(newValue)
          )
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_wheels_stick_flicking_input_window_milliseconds(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers
          )
        }
      )

      gamePadStickXFormula = OptionalSettingValue<String>(
        hasFunction: {
          return
            libkrbn_core_configuration_has_selected_profile_device_game_pad_stick_x_formula(
              Settings.shared.libkrbnCoreConfiguration,
              connectedDevice.libkrbnDeviceIdentifiers
            )
        },
        getFunction: {
          var buffer = [Int8](repeating: 0, count: 16384)
          libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_x_formula(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            &buffer,
            buffer.count
          )

          return String(cString: buffer)
        },
        setFunction: { (_ newValue: String) in
          libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_x_formula(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            newValue.cString(using: .utf8)
          )
        },
        unsetFunction: {
          libkrbn_core_configuration_unset_selected_profile_device_game_pad_stick_x_formula(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers
          )
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
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            !modifyEvents)
          Settings.shared.save()
        }
      }
    }

    @Published var manipulateCapsLockLed: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_manipulate_caps_lock_led(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            manipulateCapsLockLed)
          Settings.shared.save()
        }
      }
    }

    @Published var treatAsBuiltInKeyboard: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_treat_as_built_in_keyboard(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            treatAsBuiltInKeyboard)
          Settings.shared.save()
        }
      }
    }

    @Published var disableBuiltInKeyboardIfExists: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            disableBuiltInKeyboardIfExists)
          Settings.shared.save()
        }
      }
    }

    @Published var mouseFlipX: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_mouse_flip_x(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            mouseFlipX)
          Settings.shared.save()
        }
      }
    }

    @Published var mouseFlipY: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_mouse_flip_y(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            mouseFlipY)
          Settings.shared.save()
        }
      }
    }

    @Published var mouseFlipVerticalWheel: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_mouse_flip_vertical_wheel(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            mouseFlipVerticalWheel)
          Settings.shared.save()
        }
      }
    }
    @Published var mouseFlipHorizontalWheel: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_mouse_flip_horizontal_wheel(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            mouseFlipHorizontalWheel)
          Settings.shared.save()
        }
      }
    }

    @Published var mouseSwapXY: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_mouse_swap_xy(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            mouseSwapXY)
          Settings.shared.save()
        }
      }
    }

    @Published var mouseSwapWheels: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_mouse_swap_wheels(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            mouseSwapWheels)
          Settings.shared.save()
        }
      }
    }

    @Published var gamePadSwapSticks: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_game_pad_swap_sticks(
            Settings.shared.libkrbnCoreConfiguration,
            connectedDevice.libkrbnDeviceIdentifiers,
            gamePadSwapSticks)
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

    @Published var simpleModifications: [SimpleModification] = []
    @Published var fnFunctionKeys: [SimpleModification] = []

    public static func == (lhs: ConnectedDeviceSetting, rhs: ConnectedDeviceSetting) -> Bool {
      lhs.id == rhs.id
    }
  }
}
