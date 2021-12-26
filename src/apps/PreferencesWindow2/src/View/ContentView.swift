import SwiftUI

struct ContentView: View {
    let window: NSWindow?

    @State private var selection: String? = "SimpleModifications"

    let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as! String

    var body: some View {
        VStack {
            NavigationView {
                VStack(alignment: .leading, spacing: 0) {
                    List {
                        NavigationLink(destination: SimpleModificationsView(),
                                       tag: "SimpleModifications",
                                       selection: $selection) {
                            Label("Simple Modifications", systemImage: "gearshape")
                        }
                        .padding(10)

                        NavigationLink(destination: FunctionKeysView(),
                                       tag: "FunctionKeys",
                                       selection: $selection) {
                            Label("Function Keys", systemImage: "speaker.wave.2.circle")
                        }
                        .padding(10)

                        NavigationLink(destination: ComplexModificationsView(),
                                       tag: "ComplexModifications",
                                       selection: $selection) {
                            Label("Complex Modifications", systemImage: "gearshape.2")
                        }
                        .padding(10)

                        NavigationLink(destination: DevicesView(),
                                       tag: "Devices",
                                       selection: $selection) {
                            Label("Devices", systemImage: "keyboard")
                        }
                        .padding(10)

                        NavigationLink(destination: VirtualKeyboardView(),
                                       tag: "VirtualKeyboard",
                                       selection: $selection) {
                            Label("Virtual Keyboard", systemImage: "puzzlepiece")
                        }
                        .padding(10)

                        NavigationLink(destination: ProfilesView(),
                                       tag: "Profiles",
                                       selection: $selection) {
                            Label("Profiles", systemImage: "person.3")
                        }
                        .padding(10)

                        NavigationLink(destination: MiscView(),
                                       tag: "Misc",
                                       selection: $selection) {
                            Label("Misc", systemImage: "leaf")
                        }
                        .padding(10)

                        NavigationLink(destination: AdvancedView(),
                                       tag: "Advanced",
                                       selection: $selection) {
                            Label("Advanced", systemImage: "leaf.fill")
                        }
                        .padding(10)

                        NavigationLink(destination: LogView(),
                                       tag: "Log",
                                       selection: $selection) {
                            Label("Log", systemImage: "doc.plaintext")
                        }
                        .padding(10)

                        NavigationLink(destination: UninstallView(),
                                       tag: "Uninstall",
                                       selection: $selection) {
                            Label("Uninstall", systemImage: "trash")
                        }
                        .padding(10)
                    }
                    .listStyle(SidebarListStyle())
                    .frame(width: 250)

                    Spacer()
                }
            }
            .frame(minWidth: 1100,
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
