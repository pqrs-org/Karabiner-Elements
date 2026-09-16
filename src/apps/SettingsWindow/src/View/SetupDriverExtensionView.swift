import SwiftUI

struct SetupDriverExtensionView: View {
  @State private var showingAdvanced = false

  private let driverExtensionsImage: String

  init(showingAdvanced: Bool = false) {
    _showingAdvanced = State(initialValue: showingAdvanced)
    if #available(macOS 26.0, *) {
      driverExtensionsImage = "driver-extensions-macos26"
    } else {
      driverExtensionsImage = "driver-extensions-macos15"
    }
  }

  var body: some View {
    VStack(alignment: .center) {
      AppLocalizedLabel(
        "settings.setup.driver.permission",
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      GroupBox {
        VStack(alignment: .center, spacing: 20.0) {
          VStack(alignment: .center, spacing: 0) {
            AppLocalizedText("settings.setup.driver.not_loaded")
            AppLocalizedText(
              "settings.setup.driver.allow"
            )
          }

          OpenSystemSettingsButton(
            url: "x-apple.systempreferences:com.apple.LoginItems-Settings.extension",
            label: {
              AppLocalizedLabel(
                "settings.system_settings.open_extensions",
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
                AppLocalizedLabel(
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
                "settings.driver.step_deactivate"
              )
              .fixedSize(horizontal: false, vertical: true)

              DeactivateDriverButton()
                .padding(.vertical, 10)
                .padding(.leading, 20)

              AppLocalizedText("settings.driver.step_restart")
                .fontWeight(.bold)
                .fixedSize(horizontal: false, vertical: true)

              AppLocalizedText(
                "settings.setup.driver.step_automatic_load"
              )
              .fixedSize(horizontal: false, vertical: true)
            }
          }.padding()
        }
      }
    }
  }
}
