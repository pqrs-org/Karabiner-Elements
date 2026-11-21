import SwiftUI

struct ContentView: View {
  static let defaultContentSize = NSSize(width: 1300, height: 650)

  @AppStorage("uiLanguage") private var language = "auto"
  @AppLocalizationContext private var localized
  @ObservedObject private var localization = AppLocalization.shared
  @ObservedObject private var debugPreview = DebugAlertPreviewState.shared

  @ObservedObject private var captureCoordinator = CaptureCoordinator.shared
  @ObservedObject private var inputMonitoringAlertData = InputMonitoringAlertData.shared
  @ObservedObject private var secureEventInputState = SecureEventInputState.shared
  @StateObject private var windowReference = WindowReference()

  @State private var staysOnTop = false
  @State private var staysOnTopStatusID: UUID?

  var body: some View {
    ZStack {
      ContentMainView()
        // Disable the underlying controls to remove focus from the test input field while the Input
        // Monitoring alert is shown. Keep them enabled for the Secure Keyboard Entry warning so the
        // current focus is preserved.
        .disabled(inputMonitoringAlertData.showing)

      if inputMonitoringAlertData.showing {
        OverlayAlertView {
          InputMonitoringAlertView()
        }
      } else if secureEventInputWarningShowing {
        OverlayAlertView(showsBorder: false) {
          SecureEventInputWarningView()
        }
      }
      if let alert = debugPreview.alert {
        DebugAlertsView.previewView(alert)
          .modifier(
            LocalizationPreviewInteraction {
              debugPreview.alert = nil
            })
      }

    }
    .background {
      IconOnlyToolbarConfigurator().frame(width: 0, height: 0)
      WindowAccessor { window in
        windowReference.window = window
        if window != nil {
          setWindowProperty()
        }
      }
      .frame(width: 0, height: 0)
    }
    .onAppear {
      setWindowProperty()
      if secureEventInputWarningShowing {
        stopRawInputCaptureIfNeeded()
      }
    }
    .onChange(of: staysOnTop) { _ in
      setWindowProperty()
      staysOnTopStatusID = UUID()
    }
    .onChange(of: secureEventInputWarningShowing) { showing in
      if showing {
        stopRawInputCaptureIfNeeded()
      }
    }
    .frame(
      minWidth: Self.defaultContentSize.width,
      maxWidth: .infinity,
      minHeight: Self.defaultContentSize.height,
      maxHeight: .infinity
    )
    .toolbar {
      ToolbarItem(placement: .primaryAction) {
        HStack(spacing: 2) {
          StayOnTopButton(
            enabled: $staysOnTop,
            statusPresentationID: staysOnTopStatusID,
            statusMessage: localized(
              "event_viewer.stay_on_top.status",
              arguments: [
                "value": localized(staysOnTop ? "shared.value.on" : "shared.value.off")
              ]),
            accessibilityLabel: localized("event_viewer.settings.stay_on_top")
          )
          .task(id: staysOnTopStatusID) {
            guard let id = staysOnTopStatusID else { return }

            do {
              try await Task.sleep(for: .seconds(2))
            } catch {
              return
            }

            if staysOnTopStatusID == id {
              staysOnTopStatusID = nil
            }
          }

          Rectangle()
            .fill(Color(NSColor.separatorColor))
            .frame(width: 1, height: 16)

          LanguagePicker(selection: $language, languages: localization.catalog.languages)
            .fixedSize()
        }
      }
    }
  }

  private var secureEventInputWarningShowing: Bool {
    guard secureEventInputState.enabled else {
      return false
    }

    // Do not require `capturing` here. Raw input capture is stopped when this warning
    // appears (or when the application resigns active), so that condition would
    // immediately dismiss the warning or prevent it from appearing at all. Keeping
    // the warning visible while a device is selected also prevents Start capture from
    // being pressed while Secure Keyboard Entry is enabled.
    switch captureCoordinator.currentMode {
    case .inputEvents:
      return true

    case .rawInputEvents:
      return captureCoordinator.rawInputEventsSelectedDeviceId != nil

    case .rawInputRecords:
      return captureCoordinator.rawInputRecordsSelectedDeviceId != nil

    case nil:
      return false
    }
  }

  private func stopRawInputCaptureIfNeeded() {
    // The warning overlay prevents the user from pressing Stop capture. Raw input
    // capture temporarily disables Karabiner-Elements modifications for the selected
    // device, so stop it before displaying the overlay. Regular input capture has no
    // such side effects, so keep it active to resume capturing automatically when
    // Secure Keyboard Entry is disabled.
    switch captureCoordinator.currentMode {
    case .rawInputEvents, .rawInputRecords:
      captureCoordinator.stopCapture()

    case .inputEvents, nil:
      break
    }
  }

  private func setWindowProperty() {
    guard let window = windowReference.window else { return }

    if staysOnTop {
      window.level = .floating
    } else {
      window.level = .normal
    }

  }
}

@MainActor
private final class WindowReference: ObservableObject {
  weak var window: NSWindow?
}

private struct WindowAccessor: NSViewRepresentable {
  let onWindowChange: @MainActor (NSWindow?) -> Void

  func makeNSView(context: Context) -> AccessorView {
    AccessorView(onWindowChange: onWindowChange)
  }

  func updateNSView(_ view: AccessorView, context: Context) {
    view.onWindowChange = onWindowChange
  }

  static func dismantleNSView(_ view: AccessorView, coordinator: ()) {
    view.onWindowChange(nil)
  }

  @MainActor
  final class AccessorView: NSView {
    var onWindowChange: @MainActor (NSWindow?) -> Void

    init(onWindowChange: @escaping @MainActor (NSWindow?) -> Void) {
      self.onWindowChange = onWindowChange
      super.init(frame: .zero)
    }

    required init?(coder: NSCoder) {
      fatalError("init(coder:) has not been implemented")
    }

    override func viewDidMoveToWindow() {
      super.viewDidMoveToWindow()
      onWindowChange(window)
    }
  }
}

private struct StayOnTopButton: NSViewRepresentable {
  @Binding var enabled: Bool
  let statusPresentationID: UUID?
  let statusMessage: String
  let accessibilityLabel: String

  func makeCoordinator() -> Coordinator {
    Coordinator(enabled: $enabled)
  }

  func makeNSView(context: Context) -> NSButton {
    let button = NSButton()
    button.setButtonType(.toggle)
    button.bezelStyle = .toolbar
    button.imagePosition = .imageOnly
    button.image = NSImage(systemSymbolName: "pin", accessibilityDescription: nil)
    button.target = context.coordinator
    button.action = #selector(Coordinator.buttonPressed(_:))
    button.setContentHuggingPriority(.required, for: .horizontal)
    button.setContentCompressionResistancePriority(.required, for: .horizontal)
    return button
  }

  func updateNSView(_ button: NSButton, context: Context) {
    context.coordinator.enabled = $enabled

    button.state = enabled ? .on : .off
    button.image = NSImage(
      systemSymbolName: enabled ? "pin.fill" : "pin",
      accessibilityDescription: accessibilityLabel)
    button.toolTip = accessibilityLabel
    button.setAccessibilityLabel(accessibilityLabel)

    context.coordinator.updateStatusPanel(
      presentationID: statusPresentationID,
      message: statusMessage,
      systemImage: enabled ? "pin.fill" : "pin",
      relativeTo: button)
  }

  func sizeThatFits(_ proposal: ProposedViewSize, nsView: NSButton, context: Context) -> CGSize? {
    nsView.intrinsicContentSize
  }

  static func dismantleNSView(_ button: NSButton, coordinator: Coordinator) {
    coordinator.dismiss()
  }

  @MainActor
  final class Coordinator: NSObject {
    var enabled: Binding<Bool>

    private let statusPanel = TransientStatusPanel()

    init(enabled: Binding<Bool>) {
      self.enabled = enabled
    }

    @objc func buttonPressed(_ button: NSButton) {
      enabled.wrappedValue = button.state == .on
    }

    func updateStatusPanel(
      presentationID: UUID?,
      message: String,
      systemImage: String,
      relativeTo button: NSButton
    ) {
      statusPanel.update(
        presentationID: presentationID,
        message: message,
        systemImage: systemImage,
        relativeTo: button)
    }

    func dismiss() {
      statusPanel.dismiss()
    }
  }
}
