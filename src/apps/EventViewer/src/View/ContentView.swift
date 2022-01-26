import SwiftUI

struct ContentView: View {
  let window: NSWindow?

  @ObservedObject var inputMonitoringAlertData = InputMonitoringAlertData.shared
  @State private var selection: String? = "Main"

  let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as! String

  var body: some View {
    VStack {
      if inputMonitoringAlertData.showing {
        InputMonitoringAlertView(window: window)
      } else {
        NavigationView {
          VStack(alignment: .leading, spacing: 0) {
            List {
              NavigationLink(
                destination: MainView(),
                tag: "Main",
                selection: $selection
              ) {
                Label("Main", systemImage: "magnifyingglass")
              }
              .padding(10)

              NavigationLink(
                destination: FrontmostApplicationView(),
                tag: "FrontmostApplication",
                selection: $selection
              ) {
                Label("Frontmost Application", systemImage: "triangle.circle")
              }
              .padding(10)

              NavigationLink(
                destination: VariablesView(),
                tag: "Variables",
                selection: $selection
              ) {
                Label("Variables", systemImage: "cube")
              }
              .padding(10)

              NavigationLink(
                destination: DevicesView(),
                tag: "Devices",
                selection: $selection
              ) {
                Label("Devices", systemImage: "keyboard")
              }
              .padding(10)

              NavigationLink(
                destination: SystemExtensionsView(),
                tag: "SystemExtensions",
                selection: $selection
              ) {
                Label("System Extensions", systemImage: "puzzlepiece")
              }
              .padding(10)

              NavigationLink(
                destination: UnknownEventsView(),
                tag: "UnknownEvents",
                selection: $selection
              ) {
                Label("Unknown Events", systemImage: "questionmark.square.dashed")
              }
              .padding(10)

              NavigationLink(
                destination: PreferencesView(),
                tag: "Preferences",
                selection: $selection
              ) {
                Label("Preferences", systemImage: "gearshape")
              }
              .padding(10)
            }
            .listStyle(SidebarListStyle())
            .frame(width: 250)

            Spacer()
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
}

struct ContentView_Previews: PreviewProvider {
  static var previews: some View {
    ContentView(window: nil)
  }
}
