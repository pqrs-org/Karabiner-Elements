import SwiftUI

struct MainView: View {
    @ObservedObject var eventQueue = EventQueue.shared

    var body: some View {
        VStack(alignment: .leading, spacing: 25.0) {
            GroupBox(label: Text("Keyboard & pointing events")) {
                VStack(alignment: .leading, spacing: 10.0) {
                    ScrollViewReader { proxy in
                        // swiftformat:disable:next unusedArguments
                        List($eventQueue.queue) { $entry in
                            VStack {
                                HStack(alignment: .center, spacing: 0) {
                                    Text(entry.eventType)
                                        .font(.title)
                                        .frame(width: 70, alignment: .leading)

                                    VStack(spacing: 0) {
                                        Text(entry.name)
                                            .truncationMode(.tail)
                                            .lineLimit(1)
                                        Text(entry.misc)
                                    }

                                    Spacer()

                                    VStack(alignment: .trailing, spacing: 0) {
                                        HStack(alignment: .bottom, spacing: 0) {
                                            Text("usage page: ").font(.caption)
                                            Text(entry.usagePage)
                                        }
                                        HStack(alignment: .bottom, spacing: 0) {
                                            Text("usage: ").font(.caption)
                                            Text(entry.usage)
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
                                proxy.scrollTo(last.id)
                            }
                        }
                    }
                }
                .padding()
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
