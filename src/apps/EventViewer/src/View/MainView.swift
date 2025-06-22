import SwiftUI

struct MainView: View {
  @ObservedObject var eventHistory = EventHistory.shared
  @State private var textInput: String = "Press the key you want to investigate."
  @State private var monitoring = true

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      GroupBox(label: Text("Text input test field")) {
        VStack {
          TextEditor(
            text: $textInput
          )
          .frame(height: 60)
          .disableAutocorrection(true)
        }
        .padding()
      }

      GroupBox(label: Text("Keyboard & pointing events")) {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack(alignment: .center, spacing: 12.0) {
            Button(
              action: {
                eventHistory.copyToPasteboard()
              },
              label: {
                Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
              }
            )
            .disabled(eventHistory.entries.isEmpty)

            Button(
              action: {
                eventHistory.clear()
              },
              label: {
                Label("Clear", systemImage: "clear")
              }
            )
            .disabled(eventHistory.entries.isEmpty)

            Spacer()

            Toggle("Monitoring events", isOn: $monitoring)
              .switchToggleStyle()
              .onChange(of: monitoring) { newValue in
                if newValue {
                  eventHistory.start()
                } else {
                  eventHistory.stop()
                }
              }
          }

          ScrollViewReader { proxy in
            ScrollView {
              VStack(alignment: .leading, spacing: 0.0) {
                ForEach($eventHistory.entries) { $entry in
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
                            .font(.custom("Menlo", size: 11.0))
                        }
                      }
                      if entry.usage.count > 0 {
                        HStack(alignment: .bottom, spacing: 0) {
                          Text("usage: ")
                            .font(.caption)
                          Text(entry.usage)
                            .font(.custom("Menlo", size: 11.0))
                        }
                      }
                    }
                    .frame(alignment: .leading)
                  }
                  .padding(.horizontal, 12.0)

                  Divider().id("divider \(entry.id)")
                }

                Spacer()
              }
            }
            .background(Color(NSColor.textBackgroundColor))
            .onAppear {
              if let last = eventHistory.entries.last {
                proxy.scrollTo("divider \(last.id)", anchor: .bottom)
              }
            }
            .onChange(of: eventHistory.entries) { newEntries in
              if let last = newEntries.last {
                proxy.scrollTo("divider \(last.id)", anchor: .bottom)
              }
            }
          }
        }
        .padding()
      }
    }
    .padding()
    .onAppear {
      eventHistory.start()
    }
    .onDisappear {
      eventHistory.stop()
    }
    .onReceive(eventHistory.$monitoring) { value in
      monitoring = value
    }
  }
}
