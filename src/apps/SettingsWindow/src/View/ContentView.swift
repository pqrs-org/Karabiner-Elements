import SwiftUI

enum NavigationTag: String {
  case simpleModifications
  case functionKeys
  case complexModifications
  case complexModificationsAdvanced
  case devices
  case virtualKeyboard
  case profiles
  case ui
  case update
  case misc
  case uninstall
  case log
  case pro
  case action
}

struct ContentView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @ObservedObject private var settings = LibKrbn.Settings.shared

  let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as! String
  let padding = 6.0

  var body: some View {
    VStack {
      HStack {
        VStack(alignment: .leading, spacing: 0) {
          if settings.unsafeUI {
            Button(action: {
              contentViewStates.navigationSelection = NavigationTag.pro
            }) {
              HStack {
                Spacer()

                Text("Unsafe configuration is enabled")

                Spacer()
              }
              .sidebarButtonLabelStyle()
            }
            .buttonStyle(PlainButtonStyle())
            .background(Color.red)
            .foregroundColor(.white)
            .cornerRadius(5)

            Divider()
              .padding(.vertical, 2.0)
          }

          Group {
            Button(action: {
              contentViewStates.navigationSelection = NavigationTag.simpleModifications
            }) {
              SidebarLabelView(text: "Simple Modifications", systemImage: "gearshape", padding: 2.0)
            }
            .sidebarButtonStyle(
              selected: contentViewStates.navigationSelection
                == NavigationTag.simpleModifications)

            Button(action: {
              contentViewStates.navigationSelection = NavigationTag.functionKeys
            }) {
              SidebarLabelView(
                text: "Function Keys", systemImage: "speaker.wave.2.circle", padding: 2.0)
            }
            .sidebarButtonStyle(
              selected: contentViewStates.navigationSelection == NavigationTag.functionKeys
            )

            Button(action: {
              contentViewStates.navigationSelection = NavigationTag.complexModifications
            }) {
              SidebarLabelView(
                text: "Complex Modifications", systemImage: "gearshape.2", padding: 2.0)
            }
            .sidebarButtonStyle(
              selected: contentViewStates.navigationSelection
                == NavigationTag.complexModifications)

            Button(action: {
              contentViewStates.navigationSelection =
                NavigationTag.complexModificationsAdvanced
            }) {
              SidebarLabelView(text: "Parameters", systemImage: "dial.min", padding: 2.0)
            }
            .sidebarButtonStyle(
              selected: contentViewStates.navigationSelection
                == NavigationTag.complexModificationsAdvanced)
          }

          Divider()
            .padding(.vertical, 10.0)

          Group {
            Button(action: {
              contentViewStates.navigationSelection = NavigationTag.devices
            }) {
              SidebarLabelView(text: "Devices", systemImage: "keyboard", padding: 2.0)
            }
            .sidebarButtonStyle(
              selected: contentViewStates.navigationSelection == NavigationTag.devices)

            Button(action: {
              contentViewStates.navigationSelection = NavigationTag.virtualKeyboard
            }) {
              SidebarLabelView(text: "Virtual Keyboard", systemImage: "puzzlepiece", padding: 2.0)
            }
            .sidebarButtonStyle(
              selected: contentViewStates.navigationSelection
                == NavigationTag.virtualKeyboard)

            Button(action: {
              contentViewStates.navigationSelection = NavigationTag.profiles
            }) {
              SidebarLabelView(text: "Profiles", systemImage: "person.3", padding: 2.0)
            }
            .sidebarButtonStyle(
              selected: contentViewStates.navigationSelection == NavigationTag.profiles)

            Button(action: {
              contentViewStates.navigationSelection = NavigationTag.ui
            }) {
              SidebarLabelView(text: "UI", systemImage: "switch.2", padding: 2.0)
            }
            .sidebarButtonStyle(
              selected: contentViewStates.navigationSelection == NavigationTag.ui)
          }

          Divider()
            .padding(.vertical, 10.0)

          Group {
            Button(action: {
              contentViewStates.navigationSelection = NavigationTag.update
            }) {
              SidebarLabelView(text: "Update", systemImage: "network", padding: 2.0)
            }
            .sidebarButtonStyle(
              selected: contentViewStates.navigationSelection == NavigationTag.update)

            Button(action: {
              contentViewStates.navigationSelection = NavigationTag.misc
            }) {
              SidebarLabelView(text: "Misc", systemImage: "leaf", padding: 2.0)
            }
            .sidebarButtonStyle(
              selected: contentViewStates.navigationSelection == NavigationTag.misc)

            Button(action: {
              contentViewStates.navigationSelection = NavigationTag.uninstall
            }) {
              SidebarLabelView(text: "Uninstall", systemImage: "trash", padding: 2.0)
            }
            .sidebarButtonStyle(
              selected: contentViewStates.navigationSelection == NavigationTag.uninstall)
          }

          Divider()
            .padding(.vertical, 10.0)

          Group {
            Button(action: {
              contentViewStates.navigationSelection = NavigationTag.log
            }) {
              SidebarLabelView(text: "Log", systemImage: "doc.plaintext", padding: 2.0)
            }
            .sidebarButtonStyle(
              selected: contentViewStates.navigationSelection == NavigationTag.log)

            Button(action: {
              contentViewStates.navigationSelection = NavigationTag.pro
            }) {
              SidebarLabelView(text: "Pro", systemImage: "flame", padding: 2.0)
            }
            .sidebarButtonStyle(
              selected: contentViewStates.navigationSelection == NavigationTag.pro)

            Button(action: {
              contentViewStates.navigationSelection = NavigationTag.action
            }) {
              SidebarLabelView(text: "Quit, Restart", systemImage: "bolt.circle", padding: 2.0)
            }
            .sidebarButtonStyle(
              selected: contentViewStates.navigationSelection == NavigationTag.action)
          }

          Spacer()
        }
        .frame(width: 250)

        Divider()

        VStack(alignment: .leading, spacing: 0) {
          if settings.saveErrorMessage != "" {
            VStack {
              Label(
                "Save failed:\n\(settings.saveErrorMessage)",
                systemImage: "exclamationmark.circle.fill"
              )
              .padding()
            }
            .foregroundColor(Color.errorForeground)
            .background(Color.errorBackground)
          }

          switch contentViewStates.navigationSelection {
          case NavigationTag.simpleModifications:
            SimpleModificationsView()
          case NavigationTag.functionKeys:
            FunctionKeysView()
          case NavigationTag.complexModifications:
            ComplexModificationsView()
          case NavigationTag.complexModificationsAdvanced:
            ComplexModificationsAdvancedView()
          case NavigationTag.devices:
            DevicesView()
          case NavigationTag.virtualKeyboard:
            VirtualKeyboardView()
          case NavigationTag.profiles:
            ProfilesView()
          case NavigationTag.ui:
            UIView()
          case NavigationTag.update:
            UpdateView()
          case NavigationTag.misc:
            MiscView()
          case NavigationTag.uninstall:
            UninstallView()
          case NavigationTag.log:
            LogView()
          case NavigationTag.pro:
            ProView()
          case NavigationTag.action:
            ActionView()
          }
        }
      }
    }
    .frame(
      minWidth: 1100,
      maxWidth: .infinity,
      minHeight: 680,
      maxHeight: .infinity)
  }
}

struct ContentView_Previews: PreviewProvider {
  static var previews: some View {
    ContentView()
  }
}
