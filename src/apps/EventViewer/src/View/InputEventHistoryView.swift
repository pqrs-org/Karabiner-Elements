import SwiftUI

struct InputEventHistoryActions: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var eventHistory = EventHistory.shared
  @EnvironmentObject private var userSettings: UserSettings

  var body: some View {
    HStack(alignment: .center, spacing: 12) {
      Menu {
        Button("JSON") {
          eventHistory.copyToPasteboardJSON()
        }

        Button("TSV") {
          eventHistory.copyToPasteboardTSV()
        }
      } label: {
        AppLocalizedConstrainedLabel(
          "shared.action.copy_to_pasteboard", systemImage: "arrow.right.doc.on.clipboard")
      }
      .disabled(eventHistory.entries.isEmpty)

      Button {
        eventHistory.clear()
      } label: {
        AppLocalizedConstrainedLabel("shared.action.clear", systemImage: "clear")
      }
      .disabled(eventHistory.entries.isEmpty)

      Spacer()

      Toggle(
        localized("event_viewer.capture.mouse_wheels"), isOn: $userSettings.captureMouseWheels
      )
      .toggleStyle(.checkbox)

      Toggle(
        localized("event_viewer.capture.unknown_events"), isOn: $userSettings.captureUnknownEvents
      )
      .toggleStyle(.checkbox)
    }
  }
}

struct InputEventHistoryList: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var eventHistory = EventHistory.shared
  let emptyMessage: String

  var body: some View {
    ScrollViewReader { proxy in
      ScrollView {
        if eventHistory.entries.isEmpty {
          Text(emptyMessage)
            .padding(12)
            .frame(maxWidth: .infinity, alignment: .leading)
        } else {
          VStack(alignment: .leading, spacing: 0) {
            ForEach(eventHistory.entries) { entry in
              HStack(alignment: .center, spacing: 12) {
                VStack(alignment: .leading, spacing: 2) {
                  Text(entry.isUnknownEvent ? "unknown" : entry.eventType)
                    .font(.title2)
                    .frame(width: 70, alignment: .leading)

                  Text(entry.timestampString)
                    .font(.caption)
                    .monospacedDigit()
                    .foregroundStyle(.secondary)
                }

                VStack(alignment: .leading, spacing: 2) {
                  Text(
                    entry.isUnknownEvent
                      ? localized("event_viewer.events.unsupported_usage") : entry.name)

                  if !entry.misc.isEmpty {
                    Text(entry.misc)
                      .font(.caption)
                  }

                  AppLocalizedText(
                    "event_viewer.events.from", arguments: ["product": entry.product]
                  )
                  .font(.caption)
                  .foregroundStyle(.secondary)
                }
                .frame(maxWidth: .infinity, alignment: .leading)

                Divider()

                VStack(alignment: .trailing, spacing: 0) {
                  HStack(alignment: .bottom, spacing: 0) {
                    AppLocalizedText("event_viewer.events.integer_value")
                      .font(.caption)
                    Text(entry.integerValue)
                      .font(.callout)
                      .monospaced()
                  }

                  Text("")
                    .font(.callout)
                    .monospaced()
                }
                .frame(alignment: .leading)

                Divider()

                VStack(alignment: .trailing, spacing: 0) {
                  if !entry.usagePage.isEmpty {
                    HStack(alignment: .bottom, spacing: 0) {
                      AppLocalizedText("event_viewer.events.usage_page")
                        .font(.caption)
                      Text(entry.usagePage)
                        .font(.callout)
                        .monospaced()
                    }
                  }
                  if !entry.usage.isEmpty {
                    HStack(alignment: .bottom, spacing: 0) {
                      AppLocalizedText("event_viewer.events.usage")
                        .font(.caption)
                      Text(entry.usage)
                        .font(.callout)
                        .monospaced()
                    }
                  }
                }
                .frame(alignment: .leading)
              }
              .padding(.horizontal, 12)
              .background(
                entry.isUnknownEvent
                  ? Color.orange.opacity(0.15)
                  : entry.isStateNotification ? Color.gray.opacity(0.15) : Color.clear
              )

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
      .onChange(of: eventHistory.entries) { _ in
        if let last = eventHistory.entries.last {
          proxy.scrollTo("divider \(last.id)", anchor: .bottom)
        }
      }
    }
  }
}
