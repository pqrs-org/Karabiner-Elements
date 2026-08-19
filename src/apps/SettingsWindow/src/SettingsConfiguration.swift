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
    enum NotificationWindowPosition: String, Codable {
      case topLeft = "top_left"
      case topRight = "top_right"
      case bottomLeft = "bottom_left"
      case bottomRight = "bottom_right"
    }

    struct NotificationWindowColors: Codable {
      struct Theme: Codable {
        var backgroundColor: String
        var textColor: String
      }

      var light: Theme
      var dark: Theme
    }

    var checkForUpdates: Bool
    var showInMenuBar: Bool
    var showProfileNameInMenuBar: Bool
    var showAdditionalMenuItems: Bool
    var enableNotificationWindow: Bool
    var notificationWindowPosition: NotificationWindowPosition
    var notificationWindowRespectScreenVisibleFrame: Bool
    var notificationWindowShowIcon: Bool
    var notificationWindowFontSize: Int
    var notificationWindowColors: NotificationWindowColors
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
    var index: Int
    let name: String
    let selected: Bool

    var id: Int { index }

    private enum CodingKeys: String, CodingKey {
      case index
      case name
      case selected
    }

    static func == (lhs: Profile, rhs: Profile) -> Bool {
      lhs.index == rhs.index
    }
  }

  struct SimpleModification: Codable, Identifiable {
    let index: Int
    let fromJsonString: String
    let toJsonString: String

    var id: Int { index }

    @MainActor
    var fromEntry: SimpleModificationDefinitionEntry {
      makeEntry(
        jsonString: fromJsonString,
        categories: SimpleModificationDefinitions.shared.fromCategories)
    }

    @MainActor
    func toEntry(
      categories: SimpleModificationDefinitionCategories
    ) -> SimpleModificationDefinitionEntry {
      makeEntry(jsonString: toJsonString, categories: categories)
    }

    @MainActor
    private func makeEntry(
      jsonString: String,
      categories: SimpleModificationDefinitionCategories
    ) -> SimpleModificationDefinitionEntry {
      let json = CanonicalJSON.string(fromJSONString: jsonString) ?? ""
      return SimpleModificationDefinitionEntry(
        categories.findLabel(jsonString: json),
        json,
        false)
    }
  }

  struct ComplexModificationsRule: Decodable, Identifiable {
    enum CodeType: String, Decodable {
      case json
      case javascript
    }

    var index: Int
    let description: String
    let descriptionNotes: [String]
    var enabled: Bool
    let codeString: String
    let searchText: String
    let codeType: CodeType

    var id: Int { index }

    private enum CodingKeys: String, CodingKey {
      case index
      case description
      case descriptionNotes
      case enabled
      case codeString
      case searchText
      case codeType
    }

    init(
      index: Int,
      description: String,
      descriptionNotes: [String],
      enabled: Bool,
      codeString: String,
      searchText: String,
      codeType: CodeType
    ) {
      self.index = index
      self.description = description
      self.descriptionNotes = descriptionNotes
      self.enabled = enabled
      self.codeString = codeString
      self.searchText = searchText
      self.codeType = codeType
    }
  }

  struct Device: Codable {
    var ignore: Bool
    var manipulateCapsLockLed: Bool
    var swapGraveAccentAndNonUsBackslash: Bool
    var ignoreVendorEvents: Bool
    var treatAsBuiltInKeyboard: Bool
    var disableBuiltInKeyboardIfExists: Bool
    var pointingMotionXyMultiplier: Double
    var pointingMotionWheelsMultiplier: Double
    var mouseFlipX: Bool
    var mouseFlipY: Bool
    var mouseFlipVerticalWheel: Bool
    var mouseFlipHorizontalWheel: Bool
    var mouseDiscardX: Bool
    var mouseDiscardY: Bool
    var mouseDiscardVerticalWheel: Bool
    var mouseDiscardHorizontalWheel: Bool
    var mouseSwapXy: Bool
    var mouseSwapWheels: Bool
    var gamePadSwapSticks: Bool
    var gamePadXyStickDeadzone: Double
    var gamePadXyStickDeltaMagnitudeDetectionThreshold: Double
    var gamePadXyStickContinuedMovementAbsoluteMagnitudeThreshold: Double
    var gamePadXyStickContinuedMovementIntervalMilliseconds: Int
    var gamePadWheelsStickDeadzone: Double
    var gamePadWheelsStickDeltaMagnitudeDetectionThreshold: Double
    var gamePadWheelsStickContinuedMovementAbsoluteMagnitudeThreshold: Double
    var gamePadWheelsStickContinuedMovementIntervalMilliseconds: Int
    var gamePadStickXFormula: String
    var gamePadStickYFormula: String
    var gamePadStickVerticalWheelFormula: String
    var gamePadStickHorizontalWheelFormula: String
    let simpleModifications: [SimpleModification]
    let fnFunctionKeys: [SimpleModification]

    var modifyEvents: Bool {
      get { !ignore }
      set { ignore = !newValue }
    }
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

      var rules: [ComplexModificationsRule]
      var parameters: Parameters
    }

    struct VirtualHidKeyboard: Codable {
      var keyboardTypeV2: String
      var mouseKeyXyScale: Int
      var indicateStickyModifierKeysState: Bool
    }

    var ignorePointingDeviceEventsByDefault: Bool

    var modifyPointingDeviceEventsByDefault: Bool {
      get { !ignorePointingDeviceEventsByDefault }
      set { ignorePointingDeviceEventsByDefault = !newValue }
    }
    var parameters: Parameters
    let simpleModifications: [SimpleModification]
    let fnFunctionKeys: [SimpleModification]
    var devices: [String: Device]
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
// Snapshot data updated through dedicated C++ APIs, such as rules and simple modifications,
// is not included.
struct SettingsConfigurationUpdate: Encodable {
  struct SelectedProfile: Encodable {
    struct ComplexModifications: Encodable {
      let parameters: SettingsConfiguration.SelectedProfile.ComplexModifications.Parameters
    }

    let ignorePointingDeviceEventsByDefault: Bool
    let parameters: SettingsConfiguration.SelectedProfile.Parameters
    let devices: [String: SettingsConfiguration.Device]
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
      ignorePointingDeviceEventsByDefault: configuration.selectedProfile
        .ignorePointingDeviceEventsByDefault,
      parameters: configuration.selectedProfile.parameters,
      devices: configuration.selectedProfile.devices,
      complexModifications: SelectedProfile.ComplexModifications(
        parameters: configuration.selectedProfile.complexModifications.parameters),
      virtualHidKeyboard: configuration.selectedProfile.virtualHidKeyboard)
  }
}
