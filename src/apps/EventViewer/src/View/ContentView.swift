import SwiftUI

struct ContentView: View {
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
    .onReceive(NotificationCenter.default.publisher(for: UserSettings.windowSettingChanged)) { _ in
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
      if UserSettings.shared.forceStayTop {
        window.level = .floating
      } else {
        window.level = .normal
      }

      // ----------------------------------------
      if UserSettings.shared.showInAllSpaces {
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

struct ContentView_Previews: PreviewProvider {
  static var previews: some View {
    ContentView()
  }
}
