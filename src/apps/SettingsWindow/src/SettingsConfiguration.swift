import Foundation

struct SettingsConfiguration: Decodable {
  struct DeviceDefaults: Decodable {
    let pointingMotionXyMultiplier: Double
    let pointingMotionWheelsMultiplier: Double
    let gamePadXyStickDeadzone: Double
    let gamePadXyStickDeltaMagnitudeDetectionThreshold: Double
    let gamePadXyStickContinuedMovementAbsoluteMagnitudeThreshold: Double
    let gamePadXyStickContinuedMovementIntervalMilliseconds: Int
    let gamePadWheelsStickDeadzone: Double
    let gamePadWheelsStickDeltaMagnitudeDetectionThreshold: Double
    let gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold: Double
    let gamePadWheelsStickContinuedMovementIntervalMilliseconds: Int
  }

  struct GlobalConfiguration: Codable {
    var checkForUpdates: Bool
    var showInMenuBar: Bool
    var showProfileNameInMenuBar: Bool
    var showAdditionalMenuItems: Bool
    var enableNotificationWindow: Bool
    var unsafeUi: Bool
    var filterUselessEventsFromSpecificDevices: Bool
    var reorderSameTimestampInputEventsToPrioritizeModifiers: Bool
    var enableCgeventtapFallback: Bool
  }

  struct MachineSpecific: Codable {
    var enableMultitouchExtension: Bool
    var externalEditorPath: String
  }

  struct Profile: Decodable, Identifiable, Equatable {
    let id = UUID()
    var index: Int
    let name: String
    let selected: Bool

    private enum CodingKeys: String, CodingKey {
      case index
      case name
      case selected
    }

    static func == (lhs: Profile, rhs: Profile) -> Bool {
      lhs.id == rhs.id
    }
  }

  struct SimpleModification: Decodable {
    let index: Int
    let fromJsonString: String
    let toJsonString: String
  }

  struct ComplexModificationsRule: Decodable {
    let index: Int
    let description: String
    let enabled: Bool
    let codeString: String
    let searchText: String
    let codeType: String
  }

  struct Device: Decodable {
    let ignore: Bool
    let manipulateCapsLockLed: Bool
    let ignoreVendorEvents: Bool
    let treatAsBuiltInKeyboard: Bool
    let disableBuiltInKeyboardIfExists: Bool
    let pointingMotionXyMultiplier: Double
    let pointingMotionWheelsMultiplier: Double
    let mouseFlipX: Bool
    let mouseFlipY: Bool
    let mouseFlipVerticalWheel: Bool
    let mouseFlipHorizontalWheel: Bool
    let mouseDiscardX: Bool
    let mouseDiscardY: Bool
    let mouseDiscardVerticalWheel: Bool
    let mouseDiscardHorizontalWheel: Bool
    let mouseSwapXy: Bool
    let mouseSwapWheels: Bool
    let gamePadSwapSticks: Bool
    let gamePadXyStickDeadzone: Double
    let gamePadXyStickDeltaMagnitudeDetectionThreshold: Double
    let gamePadXyStickContinuedMovementAbsoluteMagnitudeThreshold: Double
    let gamePadXyStickContinuedMovementIntervalMilliseconds: Int
    let gamePadWheelsStickDeadzone: Double
    let gamePadWheelsStickDeltaMagnitudeDetectionThreshold: Double
    let gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold: Double
    let gamePadWheelsStickContinuedMovementIntervalMilliseconds: Int
    let gamePadStickXFormula: String
    let gamePadStickYFormula: String
    let gamePadStickVerticalWheelFormula: String
    let gamePadStickHorizontalWheelFormula: String
    let simpleModifications: [SimpleModification]
    let fnFunctionKeys: [SimpleModification]
  }

  struct SelectedProfile: Decodable {
    struct Parameters: Codable {
      var delayMillisecondsBeforeOpenDevice: Int
    }

    struct ComplexModifications: Decodable {
      struct Parameters: Codable {
        var basicSimultaneousThresholdMilliseconds: Int
        var basicToIfAloneTimeoutMilliseconds: Int
        var basicToIfHeldDownThresholdMilliseconds: Int
        var basicToDelayedActionDelayMilliseconds: Int
        var mouseMotionToScrollSpeed: Int
      }

      let rules: [ComplexModificationsRule]
      var parameters: Parameters
    }

    struct VirtualHidKeyboard: Codable {
      var keyboardTypeV2: String
      var mouseKeyXyScale: Int
      var indicateStickyModifierKeysState: Bool
    }

    var parameters: Parameters
    let simpleModifications: [SimpleModification]
    let fnFunctionKeys: [SimpleModification]
    let devices: [String: Device]
    var complexModifications: ComplexModifications
    var virtualHidKeyboard: VirtualHidKeyboard
  }

  let deviceDefaults: DeviceDefaults
  var globalConfiguration: GlobalConfiguration
  var machineSpecific: MachineSpecific
  var profiles: [Profile]
  var selectedProfile: SelectedProfile
}

// This is intentionally limited to the values that SettingsWindow edits directly.
// Read-only and derived snapshot data, such as devices and rules, is not sent back to C++.
struct SettingsConfigurationUpdate: Encodable {
  struct SelectedProfile: Encodable {
    struct ComplexModifications: Encodable {
      let parameters: SettingsConfiguration.SelectedProfile.ComplexModifications.Parameters
    }

    let parameters: SettingsConfiguration.SelectedProfile.Parameters
    let complexModifications: ComplexModifications
    let virtualHidKeyboard: SettingsConfiguration.SelectedProfile.VirtualHidKeyboard
  }

  let globalConfiguration: SettingsConfiguration.GlobalConfiguration
  let machineSpecific: SettingsConfiguration.MachineSpecific
  let selectedProfile: SelectedProfile

  init(_ configuration: SettingsConfiguration) {
    globalConfiguration = configuration.globalConfiguration
    machineSpecific = configuration.machineSpecific
    selectedProfile = SelectedProfile(
      parameters: configuration.selectedProfile.parameters,
      complexModifications: SelectedProfile.ComplexModifications(
        parameters: configuration.selectedProfile.complexModifications.parameters),
      virtualHidKeyboard: configuration.selectedProfile.virtualHidKeyboard)
  }
}
