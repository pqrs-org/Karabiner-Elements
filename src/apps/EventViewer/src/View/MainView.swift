import SwiftUI

struct MainView: View {
    @ObservedObject var eventQueue = EventQueue.shared
    @State private var textInput: String = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat."

    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
            GroupBox(label: Text("Keyboard & pointing events")) {
                VStack(alignment: .leading, spacing: 6.0) {
                    Button(action: {
                        EventQueue.shared.clear()
                    }) {
                        Label("Clear", systemImage: "clear")
                    }

                    ScrollViewReader { proxy in
                        // swiftformat:disable:next unusedArguments
                        List($eventQueue.queue) { $entry in
                            VStack {
                                HStack(alignment: .center, spacing: 0) {
                                    Text(entry.eventType)
                                        .font(.title)
                                        .frame(width: 70, alignment: .leading)

                                    VStack(alignment: .leading, spacing: 0) {
                                        Text(entry.name)
                                        Text(entry.misc)
                                            .font(.caption)
                                    }

                                    Spacer()

                                    VStack(alignment: .trailing, spacing: 0) {
                                        if entry.usagePage.count > 0 {
                                            HStack(alignment: .bottom, spacing: 0) {
                                                Text("usage page: ")
                                                    .font(.caption)
                                                Text(entry.usagePage)
                                            }
                                        }
                                        if entry.usage.count > 0 {
                                            HStack(alignment: .bottom, spacing: 0) {
                                                Text("usage: ")
                                                    .font(.caption)
                                                Text(entry.usage)
                                            }
                                        }
                                    }
                                    .frame(alignment: .leading)
                                }
                                .padding(.horizontal, 8)

                                Divider()
                            }
                        }
                        .onChange(of: eventQueue.queue) { newQueue in
                            if let last = newQueue.last {
                                proxy.scrollTo(last.id, anchor: .bottom)
                            }
                        }
                    }
                }
                .padding(6.0)
            }

            GroupBox(label: Text("Text input test field")) {
                VStack {
                    TextEditor(
                        text: $textInput
                    )
                    .frame(height: 60)
                    .disableAutocorrection(true)
                }
                .padding(6.0)
            }
        }
        .padding()
    }
}

struct MainView_Previews: PreviewProvider {
    static var previews: some View {
        MainView()
    }
}
