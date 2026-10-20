import SwiftUI

enum SetupDriverPresentation {
  // Case names indicate the first supported macOS version that uses each presentation.
  // The image asset may have been captured on a later version with the same layout.
  case macOS13
  case macOS15
  case macOS26

  static var current: Self {
    if #available(macOS 26.0, *) {
      .macOS26
    } else if #available(macOS 15.0, *) {
      .macOS15
    } else {
      .macOS13
    }
  }

  var imageName: String {
    switch self {
    case .macOS13:
      "dext-allow-macos14"
    case .macOS15:
      "driver-extensions-macos15"
    case .macOS26:
      "driver-extensions-macos26"
    }
  }
}

struct SetupDriverExtensionView: View {
  @State private var showingAdvanced = false

  private let driverExtensionsImage: String

  init(
    showingAdvanced: Bool = false,
    presentation: SetupDriverPresentation = .current
  ) {
    _showingAdvanced = State(initialValue: showingAdvanced)
    driverExtensionsImage = presentation.imageName
  }

  var body: some View {
    VStack(alignment: .leading) {
      AppLocalizedLabel(
        "settings.setup.driver.permission",
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      GroupBox {
        VStack(alignment: .leading, spacing: 20.0) {
          VStack(alignment: .leading, spacing: 0) {
            AppLocalizedText("settings.setup.driver.not_loaded")
            AppLocalizedText(
              "settings.setup.driver.allow"
            )
          }

          OpenSystemSettingsButton(
            url: "x-apple.systempreferences:com.apple.LoginItems-Settings.extension",
            label: {
              AppLocalizedConstrainedLabel(
                "settings.setup.system_settings.open_extensions",
                systemImage: "arrow.forward.circle.fill")
            }
          )

          Image(decorative: driverExtensionsImage)
            .resizable()
            .scaledToFit()
            .frame(height: 300)
            .border(Color.gray, width: 1)

          if !showingAdvanced {
            Button(
              action: { showingAdvanced = true },
              label: {
                AppLocalizedConstrainedLabel(
                  "settings.setup.driver.missing_extensions",
                  systemImage: "questionmark.circle")
              })
          }
        }.padding()
      }

      if showingAdvanced {
        GroupBox(label: AppLocalizedText("shared.section.advanced")) {
          VStack(alignment: .leading, spacing: 20.0) {
            VStack(alignment: .leading, spacing: 0) {
              AppLocalizedText(
                "settings.setup.driver.missing_extensions_description"
              )
              AppLocalizedText(
                "settings.setup.driver.reactivate_hint"
              )
            }

            AppLocalizedText("settings.setup.driver.reactivate_steps")

            VStack(alignment: .leading, spacing: 10.0) {
              AppLocalizedText(
                "settings.setup.driver.manual_driver_load_step_001_deactivate"
              )

              DeactivateDriverButton()
                .padding(.vertical, 10)
                .padding(.leading, 20)

              AppLocalizedText("settings.setup.driver.manual_driver_load_step_002_restart")
                .fontWeight(.bold)

              AppLocalizedText(
                "settings.setup.driver.manual_driver_load_step_003_automatic_load"
              )
            }
          }.padding()
        }
      }
    }
  }
}
