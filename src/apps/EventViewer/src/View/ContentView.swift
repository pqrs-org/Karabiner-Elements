import SwiftUI

struct ContentView: View {
    @ObservedObject var inputMonitoringAlertData = InputMonitoringAlertData.shared
    @State private var selection: String? = "Main"

    let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as! String

    var body: some View {
        VStack {
            NavigationView {
                VStack(alignment: .leading, spacing: 0) {
                    List {
                        NavigationLink(destination: MainView(),
                                       tag: "Main",
                                       selection: $selection) {
                            Label("Main", systemImage: "magnifyingglass")
                        }
                        .padding(10)

                        NavigationLink(destination: FrontmostApplicationView(),
                                       tag: "FrontmostApplication",
                                       selection: $selection) {
                            Label("Frontmost Application", systemImage: "triangle.circle")
                        }
                        .padding(10)

                        NavigationLink(destination: VariablesView(),
                                       tag: "Variables",
                                       selection: $selection) {
                            Label("Variables", systemImage: "cube")
                        }
                        .padding(10)

                        NavigationLink(destination: DevicesView(),
                                       tag: "Devices",
                                       selection: $selection) {
                            Label("Devices", systemImage: "keyboard")
                        }
                        .padding(10)

                        NavigationLink(destination: UnknownEventsView(),
                                       tag: "UnknownEvents",
                                       selection: $selection) {
                            Label("Unknown Events", systemImage: "questionmark.square.dashed")
                        }
                        .padding(10)

                        NavigationLink(destination: PreferencesView(),
                                       tag: "Preferences",
                                       selection: $selection) {
                            Label("Preferences", systemImage: "gearshape")
                        }
                        .padding(10)
                    }
                    .listStyle(SidebarListStyle())
                    .frame(width: 250)

                    Spacer()
                }
            }
        }
        .frame(minWidth: 1100,
               maxWidth: .infinity,
               minHeight: 650,
               maxHeight: .infinity)
        .sheet(isPresented: $inputMonitoringAlertData.showing) {
            ZStack(alignment: .topLeading) {
                InputMonitoringAlertView()

                Button(
                    action: {
                        inputMonitoringAlertData.showing = false
                    }
                ) {
                    Image(systemName: "xmark.circle")
                        .resizable()
                        .frame(width: 24.0, height: 24.0)
                        .foregroundColor(Color(NSColor.textColor))
                }
                .buttonStyle(PlainButtonStyle())
                .offset(x: 10, y: 10)
            }
        }
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
