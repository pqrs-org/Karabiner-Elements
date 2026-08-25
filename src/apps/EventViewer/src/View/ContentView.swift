import SwiftUI

struct ContentView: View {
  @EnvironmentObject private var userSettings: UserSettings

  @ObservedObject private var captureCoordinator = CaptureCoordinator.shared
  @ObservedObject private var inputMonitoringAlertData = InputMonitoringAlertData.shared
  @ObservedObject private var secureEventInputState = SecureEventInputState.shared

  var body: some View {
    ZStack {
      ContentMainView()

      if inputMonitoringAlertData.showing {
        OverlayAlertView {
          InputMonitoringAlertView()
        }
      } else if secureEventInputWarningShowing {
        OverlayAlertView(showsBorder: false) {
          SecureEventInputWarningView()
        }
      }
    }
    .onAppear {
      setWindowProperty()
      if secureEventInputWarningShowing {
        stopRawInputCaptureIfNeeded()
      }
    }
    .onReceive(userSettings.objectWillChange) { _ in
      Task { @MainActor in
        setWindowProperty()
      }
    }
    .onChange(of: secureEventInputWarningShowing) { showing in
      if showing {
        stopRawInputCaptureIfNeeded()
      }
    }
    .frame(
      minWidth: 1300,
      maxWidth: .infinity,
      minHeight: 650,
      maxHeight: .infinity)
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
    if let window = NSApp.windows.first {
      if userSettings.forceStayTop {
        window.level = .floating
      } else {
        window.level = .normal
      }

      // ----------------------------------------
      if userSettings.showInAllSpaces {
        window.collectionBehavior.insert(.canJoinAllSpaces)
      } else {
        window.collectionBehavior.remove(.canJoinAllSpaces)
      }

      window.collectionBehavior.insert(.managed)
      window.collectionBehavior.remove(.moveToActiveSpace)
      window.collectionBehavior.remove(.transient)
    }
  }
}
