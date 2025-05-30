import SwiftUI

struct ContentView: View {
  @EnvironmentObject private var userSettings: UserSettings

  @ObservedObject var inputMonitoringAlertData = InputMonitoringAlertData.shared

  var body: some View {
    ZStack {
      ContentMainView()

      if inputMonitoringAlertData.showing {
        OverlayAlertView {
          InputMonitoringAlertView()
        }
      }
    }
    .onAppear {
      setWindowProperty()
    }
    .onReceive(userSettings.objectWillChange) { _ in
      Task { @MainActor in
        setWindowProperty()
      }
    }
    .frame(
      minWidth: 1100,
      maxWidth: .infinity,
      minHeight: 650,
      maxHeight: .infinity)
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
