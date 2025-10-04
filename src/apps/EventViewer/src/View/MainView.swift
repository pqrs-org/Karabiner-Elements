import SwiftUI

struct MainView: View {
  @ObservedObject var eventHistory = EventHistory.shared
  @State private var textInput: String = "This text input field is used to inspect key events."
  @State private var monitoring = true

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      VStack(alignment: .leading, spacing: 12.0) {
        Label(
          "Please enter text in the field below.",
          systemImage: InfoBorder.icon
        )

        BorderedTextEditor(
          text: $textInput
        )
        .frame(height: 60)
        .disableAutocorrection(true)

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
              eventHistory.pause(!newValue)
            }
        }
      }
      .padding()

      ScrollViewReader { proxy in
        ScrollView {
          if eventHistory.entries.count == 0 {
            Text(
              "Please enter keyboard input. Using the text area above makes it easier to verify."
            )
            .padding(12.0)
            .frame(maxWidth: .infinity, alignment: .leading)

          } else {
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
                  .frame(maxWidth: .infinity, alignment: .leading)

                  VStack(alignment: .trailing, spacing: 0) {
                    if entry.usagePage.count > 0 {
                      HStack(alignment: .bottom, spacing: 0) {
                        Text("usage page: ")
                          .font(.caption)
                        Text(entry.usagePage)
                          .font(.callout)
                          .monospaced()
                      }
                    }
                    if entry.usage.count > 0 {
                      HStack(alignment: .bottom, spacing: 0) {
                        Text("usage: ")
                          .font(.caption)
                        Text(entry.usage)
                          .font(.callout)
                          .monospaced()
                      }
                    }
                  }
                  .frame(alignment: .leading)
                }
                .padding(.horizontal, 12.0)

                Divider().id("divider \(entry.id)")
              }
            }
          }
        }
        .background(Color(NSColor.textBackgroundColor))
        .border(Color(NSColor.separatorColor), width: 2)
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
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
    .task {
      eventHistory.start()
      eventHistory.pause(false)
      defer { eventHistory.stop() }

      do {
        while true {
          try Task.checkCancellation()
          try await Task.sleep(for: .seconds(1))
        }
      } catch {
      }
    }
  }
}
