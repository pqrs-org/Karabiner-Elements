import Foundation

extension LibKrbn {
  final class ConnectedDeviceSetting: Identifiable, Equatable, ObservableObject {
    private var didSetEnabled = false

    var id = UUID()
    var connectedDevice: ConnectedDevice

    init(_ connectedDevice: ConnectedDevice) {
      self.connectedDevice = connectedDevice

      updateProperties()
    }

    public func updateProperties() {
      didSetEnabled = false

      let ignore = libkrbn_core_configuration_get_selected_profile_device_ignore(
        connectedDevice.libkrbnDeviceIdentifiers)
      modifyEvents = !ignore

      manipulateCapsLockLed =
        libkrbn_core_configuration_get_selected_profile_device_manipulate_caps_lock_led(
          connectedDevice.libkrbnDeviceIdentifiers)

      treatAsBuiltInKeyboard =
        libkrbn_core_configuration_get_selected_profile_device_treat_as_built_in_keyboard(
          connectedDevice.libkrbnDeviceIdentifiers)

      disableBuiltInKeyboardIfExists =
        libkrbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(
          connectedDevice.libkrbnDeviceIdentifiers)

      //
      // mouseFlipXXX
      //

      mouseFlipX =
        libkrbn_core_configuration_get_selected_profile_device_mouse_flip_x(
          connectedDevice.libkrbnDeviceIdentifiers)

      mouseFlipY =
        libkrbn_core_configuration_get_selected_profile_device_mouse_flip_y(
          connectedDevice.libkrbnDeviceIdentifiers)

      mouseFlipVerticalWheel =
        libkrbn_core_configuration_get_selected_profile_device_mouse_flip_vertical_wheel(
          connectedDevice.libkrbnDeviceIdentifiers)

      mouseFlipHorizontalWheel =
        libkrbn_core_configuration_get_selected_profile_device_mouse_flip_horizontal_wheel(
          connectedDevice.libkrbnDeviceIdentifiers)

      //
      // mouseDiscardXXX
      //

      mouseDiscardX =
        libkrbn_core_configuration_get_selected_profile_device_mouse_discard_x(
          connectedDevice.libkrbnDeviceIdentifiers)

      mouseDiscardY =
        libkrbn_core_configuration_get_selected_profile_device_mouse_discard_y(
          connectedDevice.libkrbnDeviceIdentifiers)

      mouseDiscardVerticalWheel =
        libkrbn_core_configuration_get_selected_profile_device_mouse_discard_vertical_wheel(
          connectedDevice.libkrbnDeviceIdentifiers)

      mouseDiscardHorizontalWheel =
        libkrbn_core_configuration_get_selected_profile_device_mouse_discard_horizontal_wheel(
          connectedDevice.libkrbnDeviceIdentifiers)

      //
      // mouseSwapXXX
      //

      mouseSwapXY =
        libkrbn_core_configuration_get_selected_profile_device_mouse_swap_xy(
          connectedDevice.libkrbnDeviceIdentifiers)

      mouseSwapWheels =
        libkrbn_core_configuration_get_selected_profile_device_mouse_swap_wheels(
          connectedDevice.libkrbnDeviceIdentifiers)

      //
      // gamePadXXX
      //

      gamePadSwapSticks =
        libkrbn_core_configuration_get_selected_profile_device_game_pad_swap_sticks(
          connectedDevice.libkrbnDeviceIdentifiers)

      gamePadXYStickDeadzone =
        libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_deadzone(
          connectedDevice.libkrbnDeviceIdentifiers)

      gamePadXYStickDeltaMagnitudeDetectionThreshold =
        libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_delta_magnitude_detection_threshold(
          connectedDevice.libkrbnDeviceIdentifiers)

      gamePadXYStickContinuedMovementAbsoluteMagnitudeThreshold =
        libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(
          connectedDevice.libkrbnDeviceIdentifiers)

      gamePadXYStickContinuedMovementIntervalMilliseconds = Int(
        libkrbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(
          connectedDevice.libkrbnDeviceIdentifiers))

      gamePadWheelsStickDeadzone =
        libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_deadzone(
          connectedDevice.libkrbnDeviceIdentifiers)

      gamePadWheelsStickDeltaMagnitudeDetectionThreshold =
        libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_delta_magnitude_detection_threshold(
          connectedDevice.libkrbnDeviceIdentifiers)

      gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold =
        libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(
          connectedDevice.libkrbnDeviceIdentifiers)

      gamePadWheelsStickContinuedMovementIntervalMilliseconds = Int(
        libkrbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(
          connectedDevice.libkrbnDeviceIdentifiers))

      var buffer = [Int8](repeating: 0, count: 16384)

      if libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_x_formula(
        connectedDevice.libkrbnDeviceIdentifiers, &buffer, buffer.count)
      {
        gamePadStickXFormula = String(cString: buffer).trimmingCharacters(
          in: .whitespacesAndNewlines)
      }

      if libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_y_formula(
        connectedDevice.libkrbnDeviceIdentifiers, &buffer, buffer.count)
      {
        gamePadStickYFormula = String(cString: buffer).trimmingCharacters(
          in: .whitespacesAndNewlines)
      }

      if libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_vertical_wheel_formula(
        connectedDevice.libkrbnDeviceIdentifiers, &buffer, buffer.count)
      {
        gamePadStickVerticalWheelFormula = String(cString: buffer).trimmingCharacters(
          in: .whitespacesAndNewlines)
      }

      if libkrbn_core_configuration_get_selected_profile_device_game_pad_stick_horizontal_wheel_formula(
        connectedDevice.libkrbnDeviceIdentifiers, &buffer, buffer.count)
      {
        gamePadStickHorizontalWheelFormula = String(cString: buffer).trimmingCharacters(
          in: .whitespacesAndNewlines)
      }

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

    @Published var mouseDiscardX: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_mouse_discard_x(
            connectedDevice.libkrbnDeviceIdentifiers, mouseDiscardX)

          Settings.shared.save()
        }
      }
    }

    @Published var mouseDiscardY: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_mouse_discard_y(
            connectedDevice.libkrbnDeviceIdentifiers, mouseDiscardY)

          Settings.shared.save()
        }
      }
    }

    @Published var mouseDiscardVerticalWheel: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_mouse_discard_vertical_wheel(
            connectedDevice.libkrbnDeviceIdentifiers, mouseDiscardVerticalWheel)

          Settings.shared.save()
        }
      }
    }
    @Published var mouseDiscardHorizontalWheel: Bool = false {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_mouse_discard_horizontal_wheel(
            connectedDevice.libkrbnDeviceIdentifiers, mouseDiscardHorizontalWheel)

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

    @Published var gamePadXYStickDeadzone: Double = 0.0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_deadzone(
            connectedDevice.libkrbnDeviceIdentifiers,
            gamePadXYStickDeadzone)

          Settings.shared.save()
        }
      }
    }

    @Published var gamePadXYStickDeltaMagnitudeDetectionThreshold: Double = 0.0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_delta_magnitude_detection_threshold(
            connectedDevice.libkrbnDeviceIdentifiers,
            gamePadXYStickDeltaMagnitudeDetectionThreshold)

          Settings.shared.save()
        }
      }
    }

    @Published var gamePadXYStickContinuedMovementAbsoluteMagnitudeThreshold: Double = 0.0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(
            connectedDevice.libkrbnDeviceIdentifiers,
            gamePadXYStickContinuedMovementAbsoluteMagnitudeThreshold)

          Settings.shared.save()
        }
      }
    }

    @Published var gamePadXYStickContinuedMovementIntervalMilliseconds: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(
            connectedDevice.libkrbnDeviceIdentifiers,
            Int32(gamePadXYStickContinuedMovementIntervalMilliseconds))

          Settings.shared.save()
        }
      }
    }

    @Published var gamePadWheelsStickDeadzone: Double = 0.0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_deadzone(
            connectedDevice.libkrbnDeviceIdentifiers,
            gamePadWheelsStickDeadzone)

          Settings.shared.save()
        }
      }
    }

    @Published var gamePadWheelsStickDeltaMagnitudeDetectionThreshold: Double = 0.0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_delta_magnitude_detection_threshold(
            connectedDevice.libkrbnDeviceIdentifiers,
            gamePadWheelsStickDeltaMagnitudeDetectionThreshold)

          Settings.shared.save()
        }
      }
    }

    @Published var gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold: Double = 0.0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(
            connectedDevice.libkrbnDeviceIdentifiers,
            gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold)

          Settings.shared.save()
        }
      }
    }

    @Published var gamePadWheelsStickContinuedMovementIntervalMilliseconds: Int = 0 {
      didSet {
        if didSetEnabled {
          libkrbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(
            connectedDevice.libkrbnDeviceIdentifiers,
            Int32(gamePadWheelsStickContinuedMovementIntervalMilliseconds))

          Settings.shared.save()
        }
      }
    }

    @Published var gamePadStickXFormulaError = false

    @Published var gamePadStickXFormula = "" {
      didSet {
        if didSetEnabled {
          if libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_x_formula(
            connectedDevice.libkrbnDeviceIdentifiers,
            gamePadStickXFormula.cString(using: .utf8))
          {
            gamePadStickXFormulaError = false
            Settings.shared.save()
          } else {
            gamePadStickXFormulaError = true
          }
        }
      }
    }

    public func resetGamePadStickXFormula() {
      libkrbn_core_configuration_reset_selected_profile_device_game_pad_stick_x_formula(
        connectedDevice.libkrbnDeviceIdentifiers)

      Settings.shared.save()

      updateProperties()
    }

    @Published var gamePadStickYFormulaError = false

    @Published var gamePadStickYFormula = "" {
      didSet {
        if didSetEnabled {
          if libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_y_formula(
            connectedDevice.libkrbnDeviceIdentifiers,
            gamePadStickYFormula.cString(using: .utf8))
          {
            gamePadStickYFormulaError = false
            Settings.shared.save()
          } else {
            gamePadStickYFormulaError = true
          }
        }
      }
    }

    public func resetGamePadStickYFormula() {
      libkrbn_core_configuration_reset_selected_profile_device_game_pad_stick_y_formula(
        connectedDevice.libkrbnDeviceIdentifiers)

      Settings.shared.save()

      updateProperties()
    }

    @Published var gamePadStickVerticalWheelFormulaError = false

    @Published var gamePadStickVerticalWheelFormula = "" {
      didSet {
        if didSetEnabled {
          if libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_vertical_wheel_formula(
            connectedDevice.libkrbnDeviceIdentifiers,
            gamePadStickVerticalWheelFormula.cString(using: .utf8))
          {
            gamePadStickVerticalWheelFormulaError = false
            Settings.shared.save()
          } else {
            gamePadStickVerticalWheelFormulaError = true
          }
        }
      }
    }

    public func resetGamePadStickVerticalWheelFormula() {
      libkrbn_core_configuration_reset_selected_profile_device_game_pad_stick_vertical_wheel_formula(
        connectedDevice.libkrbnDeviceIdentifiers)

      Settings.shared.save()

      updateProperties()
    }

    @Published var gamePadStickHorizontalWheelFormulaError = false

    @Published var gamePadStickHorizontalWheelFormula = "" {
      didSet {
        if didSetEnabled {
          if libkrbn_core_configuration_set_selected_profile_device_game_pad_stick_horizontal_wheel_formula(
            connectedDevice.libkrbnDeviceIdentifiers,
            gamePadStickHorizontalWheelFormula.cString(using: .utf8))
          {
            gamePadStickHorizontalWheelFormulaError = false
            Settings.shared.save()
          } else {
            gamePadStickHorizontalWheelFormulaError = true
          }
        }
      }
    }

    public func resetGamePadStickHorizontalWheelFormula() {
      libkrbn_core_configuration_reset_selected_profile_device_game_pad_stick_horizontal_wheel_formula(
        connectedDevice.libkrbnDeviceIdentifiers)

      Settings.shared.save()

      updateProperties()
    }

    @Published var simpleModifications: [SimpleModification] = []
    @Published var fnFunctionKeys: [SimpleModification] = []

    public static func == (lhs: ConnectedDeviceSetting, rhs: ConnectedDeviceSetting) -> Bool {
      lhs.id == rhs.id
    }
  }
}
