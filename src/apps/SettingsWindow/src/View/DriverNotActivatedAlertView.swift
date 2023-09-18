import SwiftUI

struct DriverNotActivatedAlertView: View {
  @State private var showingAdvanced = false

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center) {
        Label(
          "Please allow Karabiner-VirtualHIDDevice-Manager system software",
          systemImage: "lightbulb"
        )
        .font(.system(size: 24))

        HStack(alignment: .top, spacing: 40.0) {
          GroupBox {
            VStack(alignment: .center, spacing: 20.0) {
              VStack(alignment: .center, spacing: 0) {
                Text("The virtual keyboard and mouse driver is not loaded.")
                Text(
                  "Please allow \".Karabiner-VirtualHIDDevice-Manager\" on Privacy & Security System Settings."
                )
              }

              Button(
                action: { openSystemSettingsSecurity() },
                label: {
                  Label(
                    "Open Privacy & Security System Settings...",
                    systemImage: "arrow.forward.circle.fill")
                })

              Image(decorative: "dext-allow")
                .resizable()
                .aspectRatio(contentMode: .fit)
                .frame(height: 300)
                .border(Color.gray, width: 1)

              Button(
                action: { showingAdvanced = true },
                label: {
                  Label(
                    "If the Allow button is not displayed on Privacy & Security.",
                    systemImage: "questionmark.circle")
                })
            }.padding()
          }.frame(width: showingAdvanced ? 400 : 800)

          if showingAdvanced {
            GroupBox(label: Text("Advanced")) {
              VStack(alignment: .leading, spacing: 20.0) {
                VStack(alignment: .leading, spacing: 0) {
                  Text(
                    "If macOS failed to load the driver in the early stage, the allow button might be not shown on Privacy & Security System Settings."
                  )
                  Text(
                    "In this case, you need to reinstall the driver in order for the button to appear."
                  )
                }

                Text("How to reinstall driver:")

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
                    .fixedSize(horizontal: false, vertical: true)

                  Text("3. Press the following button to activate driver.")
                    .fixedSize(horizontal: false, vertical: true)

                  ActivateDriverButton()
                    .padding(.vertical, 10)
                    .padding(.leading, 20)

                  Text("4. \"System Extension Blocked\" alert is shown.")
                    .fixedSize(horizontal: false, vertical: true)

                  Text("5. Open Privacy & Security System Settings and press the allow button.")
                    .fixedSize(horizontal: false, vertical: true)
                }
              }.padding()
            }.frame(width: 400)
          }
        }
      }
      .frame(width: 850)

      SheetCloseButton {
        ContentViewStates.shared.showDriverNotActivatedAlert = false
      }
    }
  }

  private func openSystemSettingsSecurity() {
    let url = URL(string: "x-apple.systempreferences:com.apple.preference.security?General")!
    NSWorkspace.shared.open(url)

    NSApp.miniaturizeAll(nil)
  }
}

struct DriverNotActivatedAlertView_Previews: PreviewProvider {
  static var previews: some View {
    Group {
      DriverNotActivatedAlertView()
        .previewLayout(.sizeThatFits)
    }
  }
}
