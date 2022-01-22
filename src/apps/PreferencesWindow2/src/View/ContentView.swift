import SwiftUI

struct ContentView: View {
    let window: NSWindow?

    @State private var selection: String? = "SimpleModifications"

    let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as! String
    let padding = 6.0

    var body: some View {
        VStack {
            NavigationView {
                VStack(alignment: .leading, spacing: 0) {
                    List {
                        Group {
                            NavigationLink(destination: SimpleModificationsView(),
                                           tag: "SimpleModifications",
                                           selection: $selection) {
                                Label("Simple Modifications", systemImage: "gearshape")
                            }
                            .padding(padding)

                            NavigationLink(destination: FunctionKeysView(),
                                           tag: "FunctionKeys",
                                           selection: $selection) {
                                Label("Function Keys", systemImage: "speaker.wave.2.circle")
                            }
                            .padding(padding)

                            NavigationLink(destination: ComplexModificationsView(),
                                           tag: "ComplexModifications",
                                           selection: $selection) {
                                Label("Complex Modifications", systemImage: "gearshape.2")
                            }
                            .padding(padding)
                        }

                        Divider()

                        Group {
                            NavigationLink(destination: DevicesView(),
                                           tag: "Devices",
                                           selection: $selection) {
                                Label("Devices", systemImage: "keyboard")
                            }
                            .padding(padding)

                            NavigationLink(destination: DevicesAdvancedView(),
                                           tag: "DevicesAdvanced",
                                           selection: $selection) {
                                Label("Advanced", systemImage: "keyboard")
                            }
                            .padding(.leading, 20.0)
                            .padding(.vertical, padding)

                            NavigationLink(destination: VirtualKeyboardView(),
                                           tag: "VirtualKeyboard",
                                           selection: $selection) {
                                Label("Virtual Keyboard", systemImage: "puzzlepiece")
                            }
                            .padding(padding)

                            NavigationLink(destination: ProfilesView(),
                                           tag: "Profiles",
                                           selection: $selection) {
                                Label("Profiles", systemImage: "person.3")
                            }
                            .padding(padding)
                        }

                        Divider()

                        Group {
                            NavigationLink(destination: UpdateView(),
                                           tag: "Update",
                                           selection: $selection) {
                                Label("Update", systemImage: "network")
                            }
                            .padding(padding)

                            NavigationLink(destination: MiscView(),
                                           tag: "Misc",
                                           selection: $selection) {
                                Label("Misc", systemImage: "leaf")
                            }
                            .padding(padding)

                            NavigationLink(destination: UninstallView(),
                                           tag: "Uninstall",
                                           selection: $selection) {
                                Label("Uninstall", systemImage: "trash")
                            }
                            .padding(padding)
                        }

                        Divider()

                        Group {
                            NavigationLink(destination: LogView(),
                                           tag: "Log",
                                           selection: $selection) {
                                Label("Log", systemImage: "doc.plaintext")
                            }
                            .padding(padding)

                            NavigationLink(destination: ActionView(),
                                           tag: "Action",
                                           selection: $selection) {
                                Label("Quit, Restart", systemImage: "bolt.circle")
                            }
                            .padding(padding)
                        }
                    }
                    .listStyle(SidebarListStyle())
                    .frame(width: 250)
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
