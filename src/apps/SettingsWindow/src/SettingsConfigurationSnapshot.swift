import Foundation

struct SettingsConfigurationSnapshot: Decodable {
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

  struct GlobalConfiguration: Decodable {
    let checkForUpdates: Bool
    let showInMenuBar: Bool
    let showProfileNameInMenuBar: Bool
    let showAdditionalMenuItems: Bool
    let enableNotificationWindow: Bool
    let unsafeUi: Bool
    let filterUselessEventsFromSpecificDevices: Bool
    let reorderSameTimestampInputEventsToPrioritizeModifiers: Bool
    let enableCgeventtapFallback: Bool
  }

  struct MachineSpecific: Decodable {
    let enableMultitouchExtension: Bool
    let externalEditorPath: String
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
    struct Parameters: Decodable {
      let delayMillisecondsBeforeOpenDevice: Int
    }

    struct ComplexModifications: Decodable {
      struct Parameters: Decodable {
        let basicSimultaneousThresholdMilliseconds: Int
        let basicToIfAloneTimeoutMilliseconds: Int
        let basicToIfHeldDownThresholdMilliseconds: Int
        let basicToDelayedActionDelayMilliseconds: Int
        let mouseMotionToScrollSpeed: Int
      }

      let rules: [ComplexModificationsRule]
      let parameters: Parameters
    }

    struct VirtualHidKeyboard: Decodable {
      let keyboardTypeV2: String
      let mouseKeyXyScale: Int
      let indicateStickyModifierKeysState: Bool
    }

    let parameters: Parameters
    let simpleModifications: [SimpleModification]
    let fnFunctionKeys: [SimpleModification]
    let devices: [String: Device]
    let complexModifications: ComplexModifications
    let virtualHidKeyboard: VirtualHidKeyboard
  }

  let deviceDefaults: DeviceDefaults
  let globalConfiguration: GlobalConfiguration
  let machineSpecific: MachineSpecific
  let profiles: [Profile]
  let selectedProfile: SelectedProfile
}
