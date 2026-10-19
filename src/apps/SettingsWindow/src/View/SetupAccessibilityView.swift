import SwiftUI

enum SetupAccessibilityLocalizationKey {
  static var completed: String {
    if #available(macOS 27.0, *) {
      "settings.setup.completed.device_control_and_data_access"
    } else {
      "settings.setup.completed.accessibility"
    }
  }

  static var completedInputMonitoring: String {
    if #available(macOS 27.0, *) {
      "settings.setup.completed.input_monitoring_macos27"
    } else {
      "settings.setup.completed.input_monitoring"
    }
  }

  static var description: String {
    if #available(macOS 27.0, *) {
      "settings.setup.device_control_and_data_access.description"
    } else {
      "settings.setup.accessibility.description"
    }
  }

  static var item: String {
    if #available(macOS 27.0, *) {
      "settings.setup.item.device_control_and_data_access"
    } else {
      "settings.setup.item.accessibility"
    }
  }

  static var openSetup: String {
    if #available(macOS 27.0, *) {
      "settings.setup.open_device_control_and_data_access"
    } else {
      "settings.setup.open_accessibility"
    }
  }

  static var permission: String {
    if #available(macOS 27.0, *) {
      "settings.setup.device_control_and_data_access.permission"
    } else {
      "settings.setup.accessibility.permission"
    }
  }

  static var required: String {
    if #available(macOS 27.0, *) {
      "settings.setup.device_control_and_data_access_required"
    } else {
      "settings.setup.accessibility_required"
    }
  }
}

struct SetupAccessibilityView: View {
  private let accessibilityImage: String
  private let openSystemSettingsLocalizationKey: String

  init(showsCloseButton: Bool = false) {
    if #available(macOS 27.0, *) {
      accessibilityImage = "device-control-and-data-access"
      openSystemSettingsLocalizationKey =
        "settings.setup.system_settings.open_device_control_and_data_access"
    } else {
      accessibilityImage = "accessibility-macos26"
      openSystemSettingsLocalizationKey = "settings.setup.system_settings.open_privacy"
    }
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 20.0) {
      AppLocalizedLabel(
        SetupAccessibilityLocalizationKey.permission,
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      AppLocalizedText(
        SetupAccessibilityLocalizationKey.description
      )

      OpenSystemSettingsButton(
        url: "x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility",
        label: {
          AppLocalizedConstrainedLabel(
            openSystemSettingsLocalizationKey,
            systemImage: "arrow.forward.circle.fill")
        }
      )

      Image(decorative: accessibilityImage)
        .resizable()
        .scaledToFit()
        .border(Color.gray, width: 1)
    }
  }
}
