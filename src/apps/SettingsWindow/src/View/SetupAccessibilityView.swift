import SwiftUI

enum SetupAccessibilityPresentation {
  // Case names indicate the first supported macOS version that uses each presentation.
  // The image asset may have been captured on a later version with the same layout.
  case macOS13
  case macOS27

  static var current: Self {
    if #available(macOS 27.0, *) {
      .macOS27
    } else {
      .macOS13
    }
  }

  var completedLocalizationKey: String {
    switch self {
    case .macOS27:
      "settings.setup.completed.device_control_and_data_access"
    case .macOS13:
      "settings.setup.completed.accessibility"
    }
  }

  var completedInputMonitoringLocalizationKey: String {
    switch self {
    case .macOS27:
      "settings.setup.completed.input_monitoring_macos27"
    case .macOS13:
      "settings.setup.completed.input_monitoring"
    }
  }

  var descriptionLocalizationKey: String {
    switch self {
    case .macOS27:
      "settings.setup.device_control_and_data_access.description"
    case .macOS13:
      "settings.setup.accessibility.description"
    }
  }

  var imageName: String {
    switch self {
    case .macOS27:
      "device-control-and-data-access"
    case .macOS13:
      "accessibility-macos26"
    }
  }

  var itemLocalizationKey: String {
    switch self {
    case .macOS27:
      "settings.setup.item.device_control_and_data_access"
    case .macOS13:
      "settings.setup.item.accessibility"
    }
  }

  var openSetupLocalizationKey: String {
    switch self {
    case .macOS27:
      "settings.setup.open_device_control_and_data_access"
    case .macOS13:
      "settings.setup.open_accessibility"
    }
  }

  var openSystemSettingsLocalizationKey: String {
    switch self {
    case .macOS27:
      "settings.setup.system_settings.open_device_control_and_data_access"
    case .macOS13:
      "settings.setup.system_settings.open_privacy"
    }
  }

  var permissionLocalizationKey: String {
    switch self {
    case .macOS27:
      "settings.setup.device_control_and_data_access.permission"
    case .macOS13:
      "settings.setup.accessibility.permission"
    }
  }

  var requiredLocalizationKey: String {
    switch self {
    case .macOS27:
      "settings.setup.device_control_and_data_access_required"
    case .macOS13:
      "settings.setup.accessibility_required"
    }
  }
}

struct SetupAccessibilityView: View {
  private let presentation: SetupAccessibilityPresentation

  init(
    showsCloseButton: Bool = false,
    presentation: SetupAccessibilityPresentation = .current
  ) {
    self.presentation = presentation
  }

  var body: some View {
    VStack(alignment: .leading, spacing: 20.0) {
      AppLocalizedLabel(
        presentation.permissionLocalizationKey,
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      AppLocalizedText(
        presentation.descriptionLocalizationKey
      )

      OpenSystemSettingsButton(
        url: "x-apple.systempreferences:com.apple.preference.security?Privacy_Accessibility",
        label: {
          AppLocalizedConstrainedLabel(
            presentation.openSystemSettingsLocalizationKey,
            systemImage: "arrow.forward.circle.fill")
        }
      )

      Image(decorative: presentation.imageName)
        .resizable()
        .scaledToFit()
        .border(Color.gray, width: 1)
    }
  }
}
