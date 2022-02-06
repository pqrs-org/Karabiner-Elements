import SwiftUI

enum NavigationTag: String {
  case simpleModifications
  case functionKeys
  case complexModifications
  case complexModificationsAdvanced
  case devices
  case devicesAdvanced
  case virtualKeyboard
  case profiles
  case update
  case misc
  case uninstall
  case log
  case action
}

struct ContentView: View {
  let window: NSWindow?

  @State private var selection: String? = NavigationTag.simpleModifications.rawValue

  let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as! String
  let padding = 6.0

  var body: some View {
    VStack {
      NavigationView {
        VStack(alignment: .leading, spacing: 0) {
          List {
            Group {
              NavigationLink(
                destination: SimpleModificationsView(),
                tag: NavigationTag.simpleModifications.rawValue,
                selection: $selection
              ) {
                Label("Simple Modifications", systemImage: "gearshape")
              }
              .padding(padding)

              NavigationLink(
                destination: FunctionKeysView(),
                tag: NavigationTag.functionKeys.rawValue,
                selection: $selection
              ) {
                Label("Function Keys", systemImage: "speaker.wave.2.circle")
              }
              .padding(padding)

              NavigationLink(
                destination: ComplexModificationsView(),
                tag: NavigationTag.complexModifications.rawValue,
                selection: $selection
              ) {
                Label("Complex Modifications", systemImage: "gearshape.2")
              }
              .padding(padding)

              NavigationLink(
                destination: ComplexModificationsAdvancedView(),
                tag: NavigationTag.complexModificationsAdvanced.rawValue,
                selection: $selection
              ) {
                Label("Parameters", systemImage: "dial.min")
              }
              .padding(padding)
            }

            Divider()

            Group {
              NavigationLink(
                destination: DevicesView(),
                tag: NavigationTag.devices.rawValue,
                selection: $selection
              ) {
                Label("Devices", systemImage: "keyboard")
              }
              .padding(padding)

              NavigationLink(
                destination: DevicesAdvancedView(),
                tag: NavigationTag.devicesAdvanced.rawValue,
                selection: $selection
              ) {
                Label("Devices > Advanced", systemImage: "keyboard")
              }
              .padding(padding)

              NavigationLink(
                destination: VirtualKeyboardView(),
                tag: NavigationTag.virtualKeyboard.rawValue,
                selection: $selection
              ) {
                Label("Virtual Keyboard", systemImage: "puzzlepiece")
              }
              .padding(padding)

              NavigationLink(
                destination: ProfilesView(),
                tag: NavigationTag.profiles.rawValue,
                selection: $selection
              ) {
                Label("Profiles", systemImage: "person.3")
              }
              .padding(padding)
            }

            Divider()

            Group {
              NavigationLink(
                destination: UpdateView(),
                tag: NavigationTag.update.rawValue,
                selection: $selection
              ) {
                Label("Update", systemImage: "network")
              }
              .padding(padding)

              NavigationLink(
                destination: MiscView(),
                tag: NavigationTag.misc.rawValue,
                selection: $selection
              ) {
                Label("Misc", systemImage: "leaf")
              }
              .padding(padding)

              NavigationLink(
                destination: UninstallView(),
                tag: NavigationTag.uninstall.rawValue,
                selection: $selection
              ) {
                Label("Uninstall", systemImage: "trash")
              }
              .padding(padding)
            }

            Divider()

            Group {
              NavigationLink(
                destination: LogView(),
                tag: NavigationTag.log.rawValue,
                selection: $selection
              ) {
                Label("Log", systemImage: "doc.plaintext")
              }
              .padding(padding)

              NavigationLink(
                destination: ActionView(),
                tag: NavigationTag.action.rawValue,
                selection: $selection
              ) {
                Label("Quit, Restart", systemImage: "bolt.circle")
              }
              .padding(padding)
            }
          }
          .listStyle(SidebarListStyle())
          .frame(width: 250)
        }
      }
      .frame(
        minWidth: 1100,
        maxWidth: .infinity,
        minHeight: 650,
        maxHeight: .infinity)
    }
  }
}

struct ContentView_Previews: PreviewProvider {
  static var previews: some View {
    ContentView(window: nil)
  }
}
