import SwiftUI

struct FrontmostApplicationView: View {
    @ObservedObject var frontmostApplicationHistory = FrontmostApplicationHistory.shared

    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
            GroupBox(label: Text("Frontmost application history")) {
                VStack(alignment: .leading, spacing: 12.0) {
                    Text("You can investigate a bundle identifier and a file path of the frontmost application.")
                    Label("Please switch to apps which you want to know Bundle Identifier.", systemImage: "info.circle.fill")

                    HStack(alignment: .center, spacing: 12.0) {
                        Button(action: {
                            frontmostApplicationHistory.clear()
                        }) {
                            Label("Clear", systemImage: "clear")
                        }

                        Spacer()
                    }

                    ScrollViewReader { proxy in
                        // swiftformat:disable:next unusedArguments
                        List($frontmostApplicationHistory.entries) { $entry in
                            VStack {
                                if entry.bundleIdentifier.count > 0 {
                                    HStack(alignment: .bottom, spacing: 0) {
                                        Text("Bundle Identifier: ")
                                            .font(.caption)

                                        Text(entry.bundleIdentifier)

                                        Spacer()
                                    }
                                }

                                if entry.filePath.count > 0 {
                                    HStack(alignment: .bottom, spacing: 0) {
                                        Text("File Path: ")
                                            .font(.caption)

                                        Text(entry.filePath)

                                        Spacer()
                                    }
                                }
                            }

                            Divider().id("divider \(entry.id)")
                        }
                        .onChange(of: frontmostApplicationHistory.entries) { newEntries in
                            if let last = newEntries.last {
                                proxy.scrollTo("divider \(last.id)", anchor: .bottom)
                            }
                        }
                    }
                }
                .padding(6.0)
            }

            Spacer()
        }
        .padding()
    }
}

struct FrontmostApplicationView_Previews: PreviewProvider {
    static var previews: some View {
        FrontmostApplicationView()
    }
}
