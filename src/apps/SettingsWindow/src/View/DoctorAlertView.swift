import SwiftUI

struct DoctorAlertView: View {
  @ObservedObject private var doctor = Doctor.shared
  @FocusState var focus: Bool

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center, spacing: 20.0) {
        if !doctor.userPIDDirectoryWritable {
          Label(
            "Unable to write files to the working folder.",
            systemImage: "exclamationmark.circle.fill"
          )
          .font(.system(size: 24))

          Text(
            "Karabiner-Elements failed to write a file to $HOME/.local/share/karabiner folder.\n"
              + "Typically this is due to incorrect permissions on the $HOME/.local folder.\n"
              + "Make sure you have a permission to write $HOME/.local/share/karabiner."
          )
          .modifier(ErrorBorder())

          GroupBox(label: Text("How to check the permission")) {
            VStack(alignment: .leading, spacing: 10.0) {
              HStack {
                Text("1. Open Home folder in Finder")

                Button(
                  action: { openHomeDirectory() },
                  label: {
                    Label(
                      "Open Home folder...",
                      systemImage: "arrow.forward.circle.fill")
                  }
                )
                .focused($focus)
              }

              Text("2. Press Command + Shift + . (period) to show hidden files")

              Text("3. Find .local folder, and choose \"Get Info\" from Menu")

            }
            .padding()
          }
        }

        if !doctor.karabinerJSONParseErrorMessage.isEmpty {
          Label(
            "karabiner.json couldn't be loaded due to a parse error",
            systemImage: "exclamationmark.circle.fill"
          )
          .font(.title)

          Text("It looks like the file was edited manually and now contains invalid JSON.")

          Text(doctor.karabinerJSONParseErrorMessage)
            .modifier(ErrorBorder())
        }
      }
      .padding()
      .frame(width: 850)

      SheetCloseButton {
        ContentViewStates.shared.showDoctorAlert = false
      }
    }
    .onAppear {
      focus = true
    }
  }

  private func openHomeDirectory() {
    NSWorkspace.shared.open(FileManager.default.homeDirectoryForCurrentUser)
  }
}
