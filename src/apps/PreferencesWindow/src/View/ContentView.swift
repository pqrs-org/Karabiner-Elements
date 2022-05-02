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
  @ObservedObject private var contentViewStates = ContentViewStates.shared

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
                selection: $contentViewStates.navigationSelection
              ) {
                Label("Simple Modifications", systemImage: "gearshape")
              }
              .padding(padding)

              NavigationLink(
                destination: FunctionKeysView(),
                tag: NavigationTag.functionKeys.rawValue,
                selection: $contentViewStates.navigationSelection
              ) {
                Label("Function Keys", systemImage: "speaker.wave.2.circle")
              }
              .padding(padding)

              NavigationLink(
                destination: ComplexModificationsView(),
                tag: NavigationTag.complexModifications.rawValue,
                selection: $contentViewStates.navigationSelection
              ) {
                Label("Complex Modifications", systemImage: "gearshape.2")
              }
              .padding(padding)

              NavigationLink(
                destination: ComplexModificationsAdvancedView(),
                tag: NavigationTag.complexModificationsAdvanced.rawValue,
                selection: $contentViewStates.navigationSelection
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
                selection: $contentViewStates.navigationSelection
              ) {
                Label("Devices", systemImage: "keyboard")
              }
              .padding(padding)

              NavigationLink(
                destination: DevicesAdvancedView(),
                tag: NavigationTag.devicesAdvanced.rawValue,
                selection: $contentViewStates.navigationSelection
              ) {
                Label("Devices > Advanced", systemImage: "keyboard")
              }
              .padding(padding)

              NavigationLink(
                destination: VirtualKeyboardView(),
                tag: NavigationTag.virtualKeyboard.rawValue,
                selection: $contentViewStates.navigationSelection
              ) {
                Label("Virtual Keyboard", systemImage: "puzzlepiece")
              }
              .padding(padding)

              NavigationLink(
                destination: ProfilesView(),
                tag: NavigationTag.profiles.rawValue,
                selection: $contentViewStates.navigationSelection
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
                selection: $contentViewStates.navigationSelection
              ) {
                Label("Update", systemImage: "network")
              }
              .padding(padding)

              NavigationLink(
                destination: MiscView(),
                tag: NavigationTag.misc.rawValue,
                selection: $contentViewStates.navigationSelection
              ) {
                Label("Misc", systemImage: "leaf")
              }
              .padding(padding)

              NavigationLink(
                destination: UninstallView(),
                tag: NavigationTag.uninstall.rawValue,
                selection: $contentViewStates.navigationSelection
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
                selection: $contentViewStates.navigationSelection
              ) {
                Label("Log", systemImage: "doc.plaintext")
              }
              .padding(padding)

              NavigationLink(
                destination: ActionView(),
                tag: NavigationTag.action.rawValue,
                selection: $contentViewStates.navigationSelection
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
    ContentView()
  }
}
