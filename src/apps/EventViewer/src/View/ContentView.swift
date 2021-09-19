import SwiftUI

struct ContentView: View {
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
                            Text("Main")
                        }
                        .padding(10)

                        NavigationLink(destination: FrontmostApplicationView(),
                                       tag: "FrontmostApplication",
                                       selection: $selection) {
                            Text("FrontmostApplication")
                        }
                        .padding(10)

                        NavigationLink(destination: VariablesView(),
                                       tag: "Variables",
                                       selection: $selection) {
                            Text("Variables")
                        }
                        .padding(10)

                        NavigationLink(destination: DevicesView(),
                                       tag: "Devices",
                                       selection: $selection) {
                            Text("Devices")
                        }
                        .padding(10)

                        NavigationLink(destination: PreferencesView(),
                                       tag: "Preferences",
                                       selection: $selection) {
                            Text("Preferences")
                        }
                        .padding(10)
                    }
                    .listStyle(SidebarListStyle())
                    .frame(width: 200)

                    Spacer()
                }
            }
        }.frame(minWidth: 900, minHeight: 550)
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
