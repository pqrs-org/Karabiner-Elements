import SwiftUI

struct UnknownEventsView: View {
    @ObservedObject var eventHistory = EventHistory.shared

    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
            GroupBox(label: Text("Showing events that are not supported by Karabiner-Elements")) {
                VStack(alignment: .leading, spacing: 6.0) {
                    HStack(alignment: .center, spacing: 12.0) {
                        Button(action: {
                            eventHistory.copyToPasteboardUnknownEvents()
                        }) {
                            Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
                        }

                        Button(action: {
                            eventHistory.clearUnknownEvents()
                        }) {
                            Label("Clear", systemImage: "clear")
                        }

                        Spacer()
                    }

                    // swiftformat:disable:next unusedArguments
                    List($eventHistory.unknownEventEntries) { $entry in
                        HStack(alignment: .center, spacing: 0) {
                            Text(entry.eventType)
                                .font(.title)
                                .frame(width: 70, alignment: .leading)

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

                        Divider()
                    }
                }
                .padding(6.0)
            }
        }
        .padding()
    }
}

struct UnknownEventsView_Previews: PreviewProvider {
    static var previews: some View {
        UnknownEventsView()
    }
}
