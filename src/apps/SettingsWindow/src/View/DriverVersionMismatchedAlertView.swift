import SwiftUI

struct DriverVersionMismatchedAlertView: View {
  @State private var showingAdvanced = false
  @FocusState var focus: Bool

  init(showingAdvanced: Bool = false) {
    _showingAdvanced = State(initialValue: showingAdvanced)
  }

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .leading, spacing: 20.0) {
        AppLocalizedLabel(
          "settings.setup.driver.restart_required",
          systemImage: "lightbulb"
        )
        .font(.system(size: 24))

        VStack(alignment: .leading, spacing: 0) {
          AppLocalizedText(
            "settings.setup.driver.outdated"
          )

          AppLocalizedText(
            "settings.setup.driver.restart_to_upgrade"
          )
          .fontWeight(.bold)
        }

        if !showingAdvanced {
          Button(
            action: { showingAdvanced = true },
            label: {
              AppLocalizedConstrainedLabel(
                "settings.setup.driver.restart_did_not_help",
                systemImage: "questionmark.circle")
            }
          )
          .focused($focus)
        }

        if showingAdvanced {
          GroupBox(label: AppLocalizedText("shared.section.advanced")) {
            VStack(alignment: .leading, spacing: 10.0) {
              AppLocalizedText(
                "settings.setup.driver.deactivate_hint"
              )

              VStack(alignment: .leading, spacing: 10.0) {
                AppLocalizedText(
                  "settings.setup.driver.manual_driver_load_step_001_deactivate"
                )

                DeactivateDriverButton()
                  .padding(.vertical, 10)
                  .padding(.leading, 20)

                AppLocalizedText("settings.setup.driver.manual_driver_load_step_002_restart")
                  .fontWeight(.bold)
              }
            }
            .padding()
          }
        }
      }
      .padding()
      .frame(width: 500)

      SheetCloseButton {
        ContentViewStates.shared.dismissCurrentAlert()
      }
    }
    .onAppear {
      focus = true
    }
  }
}
