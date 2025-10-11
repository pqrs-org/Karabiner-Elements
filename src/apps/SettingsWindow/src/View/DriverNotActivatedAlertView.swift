import SwiftUI

struct DriverNotActivatedAlertView: View {
  @State private var showingAdvanced = false
  @FocusState var focus: Bool

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center) {
        Label(
          "Please grant permission for a Driver Extension",
          systemImage: "lightbulb"
        )
        .font(.system(size: 24))

        HStack(alignment: .top, spacing: 40.0) {
          GroupBox {
            VStack(alignment: .center, spacing: 20.0) {
              VStack(alignment: .center, spacing: 0) {
                Text("The virtual keyboard and mouse driver is not loaded.")
                Text(
                  "Please allow \".Karabiner-VirtualHIDDevice-Manager\" on Driver Extensions."
                )
              }

              Button(
                action: {
                  let url = URL(
                    string: "x-apple.systempreferences:com.apple.LoginItems-Settings.extension")!
                  NSWorkspace.shared.open(url)

                  NSApp.miniaturizeAll(nil)
                },
                label: {
                  Label(
                    "Open Login Items & Extensions Settings...",
                    systemImage: "arrow.forward.circle.fill")
                }
              )
              .focused($focus)

              Image(decorative: "driver-extensions-macos15-1")
                .resizable()
                .aspectRatio(contentMode: .fit)
                .frame(height: 300)
                .border(Color.gray, width: 1)

              if !showingAdvanced {
                Button(
                  action: { showingAdvanced = true },
                  label: {
                    Label(
                      "If the Driver Extensions is not displayed on Extensions.",
                      systemImage: "questionmark.circle")
                  })
              }
            }.padding()
          }.frame(width: showingAdvanced ? 400 : 800)

          if showingAdvanced {
            GroupBox(label: Text("Advanced")) {
              VStack(alignment: .leading, spacing: 20.0) {
                VStack(alignment: .leading, spacing: 0) {
                  Text(
                    "If macOS failed to load the driver in the early stage, the Driver Extensions might be not shown on Login Items & Extensions System Settings."
                  )
                  Text(
                    "In such cases, reactivating the driver may increase the chances of it loading successfully."
                  )
                }

                Text("How to reactivate driver:")

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

                  Text(
                    "3. After restarting, the driver will be automatically loaded, and the Driver Extensions settings will appear."
                  )
                  .fixedSize(horizontal: false, vertical: true)
                }
              }.padding()
            }.frame(width: 400)
          }
        }
      }
      .padding()
      .frame(width: 850)

      SheetCloseButton {
        ContentViewStates.shared.showDriverNotActivatedAlert = false
      }
    }
    .onAppear {
      focus = true
    }
  }
}
