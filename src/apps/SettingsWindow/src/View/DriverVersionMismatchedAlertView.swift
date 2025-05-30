import SwiftUI

struct DriverVersionMismatchedAlertView: View {
  @State private var showingAdvanced = false
  @FocusState var focus: Bool

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .leading, spacing: 20.0) {
        Label(
          "macOS restart required",
          systemImage: "lightbulb"
        )
        .font(.system(size: 24))

        VStack(alignment: .leading, spacing: 0) {
          Text(
            "The current virtual keyboard and mouse driver is outdated."
          )

          Text(
            "Please restart macOS to upgrade the driver."
          )
          .fontWeight(.bold)
        }

        if !showingAdvanced {
          Button(
            action: { showingAdvanced = true },
            label: {
              Label(
                "If this message still appears after restarting macOS.",
                systemImage: "questionmark.circle")
            }
          )
          .focused($focus)
        }

        if showingAdvanced {
          GroupBox(label: Text("Advanced")) {
            VStack(alignment: .leading, spacing: 10.0) {
              Text(
                "If you continue to get this message after restarting macOS, try deactivating the virtual driver once by the following steps."
              )

              VStack(alignment: .leading, spacing: 10.0) {
                Text(
                  "1. Press the following button to deactivate driver.\n(The administrator password will be required.)"
                )
                .fixedSize(horizontal: false, vertical: true)

                DeactivateDriverButton()
                  .padding(.vertical, 10)
                  .padding(.leading, 20)

                Text("2. Restart macOS.")
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
        ContentViewStates.shared.showDriverVersionMismatchedAlert = false
      }
    }
    .onAppear {
      focus = true
    }
  }
}
