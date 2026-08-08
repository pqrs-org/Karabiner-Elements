import Foundation

@MainActor
final class ConnectedDeviceSetting: Identifiable, Equatable, ObservableObject {
  private var didSetEnabled = false

  nonisolated let id = UUID()
  var connectedDevice: ConnectedDevice

  init(
    _ connectedDevice: ConnectedDevice,
    _ deviceSetting: SettingsConfigurationSnapshot.Device
  ) {
    self.connectedDevice = connectedDevice

    updateProperties(deviceSetting)
  }

  func updateProperties(_ deviceSetting: SettingsConfigurationSnapshot.Device) {
    didSetEnabled = false

    modifyEvents = !deviceSetting.ignore
    manipulateCapsLockLed = deviceSetting.manipulateCapsLockLed
    ignoreVendorEvents = deviceSetting.ignoreVendorEvents
    treatAsBuiltInKeyboard = deviceSetting.treatAsBuiltInKeyboard
    disableBuiltInKeyboardIfExists = deviceSetting.disableBuiltInKeyboardIfExists
    pointingMotionXYMultiplier = deviceSetting.pointingMotionXyMultiplier
    pointingMotionWheelsMultiplier = deviceSetting.pointingMotionWheelsMultiplier
    mouseFlipX = deviceSetting.mouseFlipX
    mouseFlipY = deviceSetting.mouseFlipY
    mouseFlipVerticalWheel = deviceSetting.mouseFlipVerticalWheel
    mouseFlipHorizontalWheel = deviceSetting.mouseFlipHorizontalWheel
    mouseDiscardX = deviceSetting.mouseDiscardX
    mouseDiscardY = deviceSetting.mouseDiscardY
    mouseDiscardVerticalWheel = deviceSetting.mouseDiscardVerticalWheel
    mouseDiscardHorizontalWheel = deviceSetting.mouseDiscardHorizontalWheel
    mouseSwapXY = deviceSetting.mouseSwapXy
    mouseSwapWheels = deviceSetting.mouseSwapWheels
    gamePadSwapSticks = deviceSetting.gamePadSwapSticks
    gamePadXYStickDeadzone = deviceSetting.gamePadXyStickDeadzone
    gamePadXYStickDeltaMagnitudeDetectionThreshold =
      deviceSetting.gamePadXyStickDeltaMagnitudeDetectionThreshold
    gamePadXYStickContinuedMovementAbsoluteMagnitudeThreshold =
      deviceSetting.gamePadXyStickContinuedMovementAbsoluteMagnitudeThreshold
    gamePadXYStickContinuedMovementIntervalMilliseconds =
      deviceSetting.gamePadXyStickContinuedMovementIntervalMilliseconds
    gamePadWheelsStickDeadzone = deviceSetting.gamePadWheelsStickDeadzone
    gamePadWheelsStickDeltaMagnitudeDetectionThreshold =
      deviceSetting.gamePadWheelsStickDeltaMagnitudeDetectionThreshold
    gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold =
      deviceSetting.gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold
    gamePadWheelsStickContinuedMovementIntervalMilliseconds =
      deviceSetting.gamePadWheelsStickContinuedMovementIntervalMilliseconds
    gamePadStickXFormula = deviceSetting.gamePadStickXFormula
    gamePadStickYFormula = deviceSetting.gamePadStickYFormula
    gamePadStickVerticalWheelFormula = deviceSetting.gamePadStickVerticalWheelFormula
    gamePadStickHorizontalWheelFormula = deviceSetting.gamePadStickHorizontalWheelFormula
    gamePadStickXFormulaError = false
    gamePadStickYFormulaError = false
    gamePadStickVerticalWheelFormulaError = false
    gamePadStickHorizontalWheelFormulaError = false

    simpleModifications = deviceSetting.simpleModifications.map {
      SimpleModification(
        index: $0.index,
        fromJsonString: $0.fromJsonString,
        toJsonString: $0.toJsonString,
        toCategories: SimpleModificationDefinitions.shared.toCategories)
    }
    fnFunctionKeys = deviceSetting.fnFunctionKeys.map {
      SimpleModification(
        index: $0.index,
        fromJsonString: $0.fromJsonString,
        toJsonString: $0.toJsonString,
        toCategories: SimpleModificationDefinitions.shared.toCategoriesWithInheritBase)
    }

    didSetEnabled = true
  }

  @Published var modifyEvents: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersJSONCString {
          krbn_core_configuration_set_selected_profile_device_ignore($0, !modifyEvents)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var manipulateCapsLockLed: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
          krbn_core_configuration_set_selected_profile_device_mouse_flip_x($0, mouseFlipX)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var mouseFlipY: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersJSONCString {
          krbn_core_configuration_set_selected_profile_device_mouse_flip_y($0, mouseFlipY)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var mouseFlipVerticalWheel: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
          krbn_core_configuration_set_selected_profile_device_mouse_swap_xy($0, mouseSwapXY)
        }

        Settings.shared.save()
      }
    }
  }

  @Published var mouseSwapWheels: Bool = false {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
        connectedDevice.withDeviceIdentifiersJSONCString {
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
    connectedDevice.withDeviceIdentifiersJSONCString {
      krbn_core_configuration_reset_selected_profile_device_game_pad_stick_x_formula($0)
    }

    Settings.shared.save()

    Settings.shared.updateProperties()
  }

  @Published var gamePadStickYFormulaError = false

  @Published var gamePadStickYFormula = "" {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersJSONCString {
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
    connectedDevice.withDeviceIdentifiersJSONCString {
      krbn_core_configuration_reset_selected_profile_device_game_pad_stick_y_formula($0)
    }

    Settings.shared.save()

    Settings.shared.updateProperties()
  }

  @Published var gamePadStickVerticalWheelFormulaError = false

  @Published var gamePadStickVerticalWheelFormula = "" {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersJSONCString {
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
    connectedDevice.withDeviceIdentifiersJSONCString {
      krbn_core_configuration_reset_selected_profile_device_game_pad_stick_vertical_wheel_formula(
        $0)
    }

    Settings.shared.save()

    Settings.shared.updateProperties()
  }

  @Published var gamePadStickHorizontalWheelFormulaError = false

  @Published var gamePadStickHorizontalWheelFormula = "" {
    didSet {
      if didSetEnabled {
        connectedDevice.withDeviceIdentifiersJSONCString {
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
    connectedDevice.withDeviceIdentifiersJSONCString {
      krbn_core_configuration_reset_selected_profile_device_game_pad_stick_horizontal_wheel_formula(
        $0)
    }

    Settings.shared.save()

    Settings.shared.updateProperties()
  }

  @Published var simpleModifications: [SimpleModification] = []
  @Published var fnFunctionKeys: [SimpleModification] = []

  nonisolated public static func == (lhs: ConnectedDeviceSetting, rhs: ConnectedDeviceSetting)
    -> Bool
  {
    lhs.id == rhs.id
  }
}
