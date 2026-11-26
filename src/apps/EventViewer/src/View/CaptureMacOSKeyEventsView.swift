import SwiftUI

struct CaptureMacOSKeyEventsView: View {
  @AppLocalizationContext private var localized
  @ObservedObject private var coordinator = CaptureCoordinator.shared
  @ObservedObject private var capture = MacOSKeyEventCapture.shared
  @EnvironmentObject private var captureSettings: CaptureSettings
  @State private var session: CaptureCoordinator.Session?
  @State private var testInput = ""
  @FocusState private var testInputFocused: Bool

  var body: some View {
    VStack(alignment: .leading, spacing: 0) {
      VStack(alignment: .leading, spacing: 12) {
        Label {
          AppLocalizedText("event_viewer.macos_key_events.description")
        } icon: {
          Image(systemName: InfoBorder.icon)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .modifier(InfoBorder())

        HStack(spacing: 12) {
          if coordinator.capturing {
            Button(role: .destructive) {
              coordinator.stopCapture()
            } label: {
              AppLocalizedConstrainedLabel("event_viewer.capture.stop", systemImage: "stop.fill")
            }

            CaptureActiveLabel(text: localized("event_viewer.macos_key_events.active"))
          } else {
            Button {
              coordinator.startCapture()
              testInputFocused = coordinator.capturing
            } label: {
              AppLocalizedConstrainedLabel(
                "event_viewer.capture.start", systemImage: "record.circle")
            }
          }
        }

        CaptureTestInputField(text: $testInput, focus: $testInputFocused)
        HStack(spacing: 12) {
          CaptureHistoryActions(
            isEmpty: capture.entries.isEmpty,
            copyJSON: capture.copyToPasteboardJSON,
            copyTSV: capture.copyToPasteboardTSV,
            clear: capture.clear)

          Spacer()

          Toggle(
            localized("event_viewer.macos_key_events.capture_repeats"),
            isOn: $captureSettings.captureMacOSKeyRepeats
          )
          .toggleStyle(.checkbox)
        }

        if capture.failedToStart {
          AppLocalizedText("event_viewer.macos_key_events.failed")
            .foregroundStyle(.secondary)
        }
      }
      .padding()

      eventList
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
    .onAppear {
      guard session == nil else { return }
      session = coordinator.begin(.macOSKeyEvents)
    }
    .onDisappear {
      if let session {
        coordinator.end(session)
        self.session = nil
      }
    }
  }

  private var emptyMessageKey: String {
    coordinator.capturing
      ? "event_viewer.capture.input_hint" : "event_viewer.macos_key_events.start_hint"
  }

  private var eventList: some View {
    VStack(alignment: .leading, spacing: 0) {
      eventRow(
        timestamp: localized("event_viewer.macos_key_events.timestamp"),
        type: localized("event_viewer.macos_key_events.type"),
        keyCode: localized("event_viewer.macos_key_events.key_code"),
        flags: localized("event_viewer.macos_key_events.flags"),
        isRepeat: localized("event_viewer.macos_key_events.repeat")
      )
      .font(.callout.weight(.semibold))

      Divider()

      ScrollViewReader { proxy in
        ScrollView {
          if capture.entries.isEmpty {
            AppLocalizedText(emptyMessageKey)
              .padding(12)
              .frame(maxWidth: .infinity, alignment: .leading)
          } else {
            LazyVStack(alignment: .leading, spacing: 0) {
              ForEach(capture.entries) { entry in
                VStack(spacing: 0) {
                  historyRow(entry)
                  Divider()
                }
              }
            }
            .textSelection(.enabled)
          }
        }
        .onChange(of: capture.entries.last?.id) { id in
          if let id {
            proxy.scrollTo(id, anchor: .bottom)
          }
        }
      }
      .background(Color(nsColor: .textBackgroundColor))
    }
  }

  @ViewBuilder
  private func historyRow(_ entry: MacOSKeyEventCapture.Entry) -> some View {
    switch entry.payload {
    case .keyEvent(let type, let keyCode, let flags, let isRepeat):
      eventRow(
        timestamp: String(entry.timestamp),
        type: type,
        keyCode: String(keyCode),
        flags: flags,
        isRepeat: isRepeat ? "1" : "0"
      )
      .font(.system(.body, design: .monospaced))

    case .interruption(let reason):
      HStack(spacing: 12) {
        Text(String(entry.timestamp))
          .font(.system(.body, design: .monospaced))
          .frame(width: 175, alignment: .leading)
        AppLocalizedLabel(
          interruptionMessageKey(reason),
          systemImage: "exclamationmark.triangle"
        )
        .frame(maxWidth: .infinity, alignment: .leading)
      }
      .padding(.horizontal, 12)
      .padding(.vertical, 5)
      .background(Color.orange.opacity(0.15))
    }
  }

  private func interruptionMessageKey(_ reason: MacOSKeyEventCapture.Entry.Interruption) -> String {
    switch reason {
    case .tapDisabledByTimeout:
      return "event_viewer.macos_key_events.interrupted_timeout"
    case .tapDisabledByUserInput:
      return "event_viewer.macos_key_events.interrupted_user_input"
    case .secureKeyboardEntryEnabled:
      return "event_viewer.macos_key_events.interrupted_secure_input"
    }
  }

  private func eventRow(
    timestamp: String, type: String, keyCode: String, flags: String, isRepeat: String
  ) -> some View {
    HStack(spacing: 12) {
      Text(timestamp).frame(width: 175, alignment: .leading)
      Text(type).frame(width: 125, alignment: .leading)
      Text(keyCode).frame(width: 85, alignment: .leading)
      Text(flags).frame(maxWidth: .infinity, alignment: .leading)
      Text(isRepeat).frame(width: 85, alignment: .leading)
    }
    .padding(.horizontal, 12)
    .padding(.vertical, 5)
  }
}
