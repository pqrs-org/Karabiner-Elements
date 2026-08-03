import Foundation

@MainActor
final class ConnectedDeviceSetting: Identifiable, Equatable, ObservableObject {
  private var didSetEnabled = false

  nonisolated let id = UUID()
  var connectedDevice: ConnectedDevice

  init(_ connectedDevice: ConnectedDevice) {
    self.connectedDevice = connectedDevice

    updateProperties()
  }

  public func updateProperties() {
    didSetEnabled = false

    connectedDevice.withDeviceIdentifiersCPointer {
      let ignore = krbn_core_configuration_get_selected_profile_device_ignore($0)
      modifyEvents = !ignore

      manipulateCapsLockLed =
        krbn_core_configuration_get_selected_profile_device_manipulate_caps_lock_led($0)

      ignoreVendorEvents =
        krbn_core_configuration_get_selected_profile_device_ignore_vendor_events($0)

      treatAsBuiltInKeyboard =
        krbn_core_configuration_get_selected_profile_device_treat_as_built_in_keyboard($0)

      disableBuiltInKeyboardIfExists =
        krbn_core_configuration_get_selected_profile_device_disable_built_in_keyboard_if_exists(
          $0)

      pointingMotionXYMultiplier =
        krbn_core_configuration_get_selected_profile_device_pointing_motion_xy_multiplier($0)

      pointingMotionWheelsMultiplier =
        krbn_core_configuration_get_selected_profile_device_pointing_motion_wheels_multiplier(
          $0)

      //
      // mouseFlipXXX
      //

      mouseFlipX =
        krbn_core_configuration_get_selected_profile_device_mouse_flip_x($0)

      mouseFlipY =
        krbn_core_configuration_get_selected_profile_device_mouse_flip_y($0)

      mouseFlipVerticalWheel =
        krbn_core_configuration_get_selected_profile_device_mouse_flip_vertical_wheel($0)

      mouseFlipHorizontalWheel =
        krbn_core_configuration_get_selected_profile_device_mouse_flip_horizontal_wheel($0)

      //
      // mouseDiscardXXX
      //

      mouseDiscardX =
        krbn_core_configuration_get_selected_profile_device_mouse_discard_x($0)

      mouseDiscardY =
        krbn_core_configuration_get_selected_profile_device_mouse_discard_y($0)

      mouseDiscardVerticalWheel =
        krbn_core_configuration_get_selected_profile_device_mouse_discard_vertical_wheel($0)

      mouseDiscardHorizontalWheel =
        krbn_core_configuration_get_selected_profile_device_mouse_discard_horizontal_wheel($0)

      //
      // mouseSwapXXX
      //

      mouseSwapXY =
        krbn_core_configuration_get_selected_profile_device_mouse_swap_xy($0)

      mouseSwapWheels =
        krbn_core_configuration_get_selected_profile_device_mouse_swap_wheels($0)

      //
      // gamePadXXX
      //

      gamePadSwapSticks =
        krbn_core_configuration_get_selected_profile_device_game_pad_swap_sticks($0)

      gamePadXYStickDeadzone =
        krbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_deadzone($0)

      gamePadXYStickDeltaMagnitudeDetectionThreshold =
        krbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_delta_magnitude_detection_threshold(
          $0)

      gamePadXYStickContinuedMovementAbsoluteMagnitudeThreshold =
        krbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(
          $0)

      gamePadXYStickContinuedMovementIntervalMilliseconds = Int(
        krbn_core_configuration_get_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(
          $0)
      )

      gamePadWheelsStickDeadzone =
        krbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_deadzone($0)

      gamePadWheelsStickDeltaMagnitudeDetectionThreshold =
        krbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_delta_magnitude_detection_threshold(
          $0)

      gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold =
        krbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(
          $0)

      gamePadWheelsStickContinuedMovementIntervalMilliseconds = Int(
        krbn_core_configuration_get_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(
          $0)
      )

      var buffer = [CChar](repeating: 0, count: 16384)

      if krbn_core_configuration_get_selected_profile_device_game_pad_stick_x_formula(
        $0, &buffer, buffer.count)
      {
        gamePadStickXFormula =
          String(utf8String: buffer)?.trimmingCharacters(in: .whitespacesAndNewlines) ?? ""
      }

      if krbn_core_configuration_get_selected_profile_device_game_pad_stick_y_formula(
        $0, &buffer, buffer.count)
      {
        gamePadStickYFormula =
          String(utf8String: buffer)?.trimmingCharacters(in: .whitespacesAndNewlines) ?? ""
      }

      if krbn_core_configuration_get_selected_profile_device_game_pad_stick_vertical_wheel_formula(
        $0, &buffer, buffer.count)
      {
        gamePadStickVerticalWheelFormula =
          String(utf8String: buffer)?.trimmingCharacters(in: .whitespacesAndNewlines) ?? ""
      }

      if krbn_core_configuration_get_selected_profile_device_game_pad_stick_horizontal_wheel_formula(
        $0, &buffer, buffer.count)
      {
        gamePadStickHorizontalWheelFormula =
          String(utf8String: buffer)?.trimmingCharacters(in: .whitespacesAndNewlines) ?? ""
      }
    }

    simpleModifications = Settings.shared.makeSimpleModifications(connectedDevice)
    fnFunctionKeys = Settings.shared.makeFnFunctionKeys(connectedDevice)

    didSetEnabled = true
  }

  @Published var modifyEvents: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_ignore($0, !modifyEvents)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var manipulateCapsLockLed: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_manipulate_caps_lock_led(
            $0, manipulateCapsLockLed)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var ignoreVendorEvents: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_ignore_vendor_events(
            $0, ignoreVendorEvents)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var treatAsBuiltInKeyboard: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_treat_as_built_in_keyboard(
            $0, treatAsBuiltInKeyboard)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var disableBuiltInKeyboardIfExists: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_disable_built_in_keyboard_if_exists(
            $0, disableBuiltInKeyboardIfExists)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var pointingMotionXYMultiplier: Double = 1.0 {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_pointing_motion_xy_multiplier(
            $0, pointingMotionXYMultiplier)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var pointingMotionWheelsMultiplier: Double = 1.0 {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_pointing_motion_wheels_multiplier(
            $0, pointingMotionWheelsMultiplier)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var mouseFlipX: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_mouse_flip_x($0, mouseFlipX)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var mouseFlipY: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_mouse_flip_y($0, mouseFlipY)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var mouseFlipVerticalWheel: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_mouse_flip_vertical_wheel(
            $0, mouseFlipVerticalWheel)
        }

        Settings.shared.save()
      }
    }
  }
  @Published var mouseFlipHorizontalWheel: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_mouse_flip_horizontal_wheel(
            $0, mouseFlipHorizontalWheel)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var mouseDiscardX: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_mouse_discard_x(
            $0, mouseDiscardX)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var mouseDiscardY: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_mouse_discard_y(
            $0, mouseDiscardY)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var mouseDiscardVerticalWheel: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_mouse_discard_vertical_wheel(
            $0, mouseDiscardVerticalWheel)
        }

        Settings.shared.save()
      }
    }
  }
  @Published var mouseDiscardHorizontalWheel: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_mouse_discard_horizontal_wheel(
            $0, mouseDiscardHorizontalWheel)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var mouseSwapXY: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_mouse_swap_xy($0, mouseSwapXY)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var mouseSwapWheels: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_mouse_swap_wheels(
            $0, mouseSwapWheels)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var gamePadSwapSticks: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_game_pad_swap_sticks(
            $0, gamePadSwapSticks)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var gamePadXYStickDeadzone: Double = 0.0 {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_deadzone(
            $0, gamePadXYStickDeadzone)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var gamePadXYStickDeltaMagnitudeDetectionThreshold: Double = 0.0 {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_delta_magnitude_detection_threshold(
            $0, gamePadXYStickDeltaMagnitudeDetectionThreshold)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var gamePadXYStickContinuedMovementAbsoluteMagnitudeThreshold: Double = 0.0 {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_absolute_magnitude_threshold(
            $0, gamePadXYStickContinuedMovementAbsoluteMagnitudeThreshold)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var gamePadXYStickContinuedMovementIntervalMilliseconds: Int = 0 {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_game_pad_xy_stick_continued_movement_interval_milliseconds(
            $0, Int32(gamePadXYStickContinuedMovementIntervalMilliseconds))
        }

        Settings.shared.save()
      }
    }
  }

  @Published var gamePadWheelsStickDeadzone: Double = 0.0 {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_deadzone(
            $0, gamePadWheelsStickDeadzone)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var gamePadWheelsStickDeltaMagnitudeDetectionThreshold: Double = 0.0 {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_delta_magnitude_detection_threshold(
            $0, gamePadWheelsStickDeltaMagnitudeDetectionThreshold)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold: Double = 0.0 {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_absolute_magnitude_threshold(
            $0, gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var gamePadWheelsStickContinuedMovementIntervalMilliseconds: Int = 0 {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          krbn_core_configuration_set_selected_profile_device_game_pad_wheels_stick_continued_movement_interval_milliseconds(
            $0, Int32(gamePadWheelsStickContinuedMovementIntervalMilliseconds))
        }

        Settings.shared.save()
      }
    }
  }

  @Published var gamePadStickXFormulaError = false

  @Published var gamePadStickXFormula = "" {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          if let cString = gamePadStickXFormula.cString(using: .utf8) {
            if krbn_core_configuration_set_selected_profile_device_game_pad_stick_x_formula(
              $0, cString)
            {
              gamePadStickXFormulaError = false
              Settings.shared.save()
            } else {
              gamePadStickXFormulaError = true
            }
          }
        }
      }
    }
  }

  public func resetGamePadStickXFormula() {
    connectedDevice.withDeviceIdentifiersCPointer {
      krbn_core_configuration_reset_selected_profile_device_game_pad_stick_x_formula($0)
    }

    Settings.shared.save()

    updateProperties()
  }

  @Published var gamePadStickYFormulaError = false

  @Published var gamePadStickYFormula = "" {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          if let cString = gamePadStickYFormula.cString(using: .utf8) {
            if krbn_core_configuration_set_selected_profile_device_game_pad_stick_y_formula(
              $0, cString)
            {
              gamePadStickYFormulaError = false
              Settings.shared.save()
            } else {
              gamePadStickYFormulaError = true
            }
          }
        }
      }
    }
  }

  public func resetGamePadStickYFormula() {
    connectedDevice.withDeviceIdentifiersCPointer {
      krbn_core_configuration_reset_selected_profile_device_game_pad_stick_y_formula($0)
    }

    Settings.shared.save()

    updateProperties()
  }

  @Published var gamePadStickVerticalWheelFormulaError = false

  @Published var gamePadStickVerticalWheelFormula = "" {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          if let cString = gamePadStickVerticalWheelFormula.cString(using: .utf8) {
            if krbn_core_configuration_set_selected_profile_device_game_pad_stick_vertical_wheel_formula(
              $0, cString)
            {
              gamePadStickVerticalWheelFormulaError = false
              Settings.shared.save()
            } else {
              gamePadStickVerticalWheelFormulaError = true
            }
          }
        }
      }
    }
  }

  public func resetGamePadStickVerticalWheelFormula() {
    connectedDevice.withDeviceIdentifiersCPointer {
      krbn_core_configuration_reset_selected_profile_device_game_pad_stick_vertical_wheel_formula(
        $0)
    }

    Settings.shared.save()

    updateProperties()
  }

  @Published var gamePadStickHorizontalWheelFormulaError = false

  @Published var gamePadStickHorizontalWheelFormula = "" {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersCPointer {
          if let cString = gamePadStickHorizontalWheelFormula.cString(using: .utf8) {
            if krbn_core_configuration_set_selected_profile_device_game_pad_stick_horizontal_wheel_formula(
              $0, cString)
            {
              gamePadStickHorizontalWheelFormulaError = false
              Settings.shared.save()
            } else {
              gamePadStickHorizontalWheelFormulaError = true
            }
          }
        }
      }
    }
  }

  public func resetGamePadStickHorizontalWheelFormula() {
    connectedDevice.withDeviceIdentifiersCPointer {
      krbn_core_configuration_reset_selected_profile_device_game_pad_stick_horizontal_wheel_formula(
        $0)
    }

    Settings.shared.save()

    updateProperties()
  }

  @Published var simpleModifications: [SimpleModification] = []
  @Published var fnFunctionKeys: [SimpleModification] = []

  nonisolated public static func == (lhs: ConnectedDeviceSetting, rhs: ConnectedDeviceSetting)
    -> Bool
  {
    lhs.id == rhs.id
  }
}
