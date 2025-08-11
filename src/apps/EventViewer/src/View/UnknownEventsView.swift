import SwiftUI

struct UnknownEventsView: View {
  @ObservedObject var eventHistory = EventHistory.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 0.0) {
      VStack(alignment: .leading, spacing: 12.0) {
        Text("Showing events that are not supported by Karabiner-Elements")
          .font(.title)

        HStack(alignment: .center, spacing: 12.0) {
          Button(
            action: {
              eventHistory.copyToPasteboardUnknownEvents()
            },
            label: {
              Label("Copy to pasteboard", systemImage: "arrow.right.doc.on.clipboard")
            }
          )
          .disabled(eventHistory.unknownEventEntries.isEmpty)

          Button(
            action: {
              eventHistory.clearUnknownEvents()
            },
            label: {
              Label("Clear", systemImage: "clear")
            }
          )
          .disabled(eventHistory.unknownEventEntries.isEmpty)
        }
      }
      .padding()
      .frame(maxWidth: .infinity, alignment: .leading)

      ScrollViewReader { proxy in
        ScrollView {
          if eventHistory.unknownEventEntries.count == 0 {
            Text("Unknown events are not being sent.")
              .padding(12.0)
              .frame(maxWidth: .infinity, alignment: .leading)

          } else {
            VStack(alignment: .leading, spacing: 0.0) {
              ForEach($eventHistory.unknownEventEntries) { $entry in
                HStack(alignment: .center, spacing: 0) {
                  Text(entry.eventType)
                    .font(.title)
                    .frame(maxWidth: .infinity, alignment: .leading)

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
            }
          }
        }
        .background(Color(NSColor.textBackgroundColor))
        .onAppear {
          if let last = eventHistory.unknownEventEntries.last {
            proxy.scrollTo("divider \(last.id)", anchor: .bottom)
          }
        }
        .onChange(of: eventHistory.unknownEventEntries) { newEntries in
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
