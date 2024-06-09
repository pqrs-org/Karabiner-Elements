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
            systemImage: "exclamationmark.triangle"
          )
          .font(.system(size: 24))

          GroupBox {
            VStack(alignment: .leading, spacing: 0.0) {
              Text(
                "Karabiner-Elements failed to write a file to $HOME/.local/share/karabiner folder."
              )
              Text("Typically this is due to incorrect permissions on the $HOME/.local folder.")
              Text("Make sure you have a permission to write $HOME/.local/share/karabiner.")
            }
            .padding()
          }

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

struct DoctorAlertView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      DoctorAlertView()
        .previewLayout(.sizeThatFits)
    }
  }
}
