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
          "settings.driver.restart_required",
          systemImage: "lightbulb"
        )
        .font(.system(size: 24))

        VStack(alignment: .leading, spacing: 0) {
          AppLocalizedText(
            "settings.driver.outdated"
          )

          AppLocalizedText(
            "settings.driver.restart_to_upgrade"
          )
          .fontWeight(.bold)
        }

        if !showingAdvanced {
          Button(
            action: { showingAdvanced = true },
            label: {
              AppLocalizedLabel(
                "settings.driver.restart_did_not_help",
                systemImage: "questionmark.circle")
            }
          )
          .focused($focus)
        }

        if showingAdvanced {
          GroupBox(label: AppLocalizedText("shared.section.advanced")) {
            VStack(alignment: .leading, spacing: 10.0) {
              AppLocalizedText(
                "settings.driver.deactivate_hint"
              )

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
