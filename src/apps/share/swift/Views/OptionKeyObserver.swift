import AppKit
import SwiftUI

// Observe only this application's modifier events; no global input monitoring
// permission is needed. The observer disappears with its owning view.
struct OptionKeyObserver: NSViewRepresentable {
  @Binding var isPressed: Bool

  func makeCoordinator() -> Coordinator { Coordinator(isPressed: $isPressed) }

  func makeNSView(context: Context) -> NSView {
    context.coordinator.start()
    return NSView(frame: .zero)
  }

  func updateNSView(_ view: NSView, context: Context) {
    context.coordinator.isPressed = $isPressed
  }

  static func dismantleNSView(_ view: NSView, coordinator: Coordinator) {
    coordinator.stop()
  }

  @MainActor
  final class Coordinator {
    var isPressed: Binding<Bool>
    private var eventMonitor: Any?
    private var notificationObservers: [NSObjectProtocol] = []

    init(isPressed: Binding<Bool>) { self.isPressed = isPressed }

    func start() {
      guard eventMonitor == nil else { return }
      // Defer the first update until after SwiftUI finishes installing the view.
      Task { @MainActor [weak self] in self?.refresh() }
      eventMonitor = NSEvent.addLocalMonitorForEvents(matching: .flagsChanged) {
        [weak self] event in
        MainActor.assumeIsolated {
          self?.isPressed.wrappedValue = event.modifierFlags.contains(.option)
        }
        return event
      }
      for name in [
        NSApplication.didBecomeActiveNotification, NSApplication.didResignActiveNotification,
      ] {
        notificationObservers.append(
          NotificationCenter.default.addObserver(forName: name, object: nil, queue: .main) {
            [weak self] _ in
            MainActor.assumeIsolated { self?.refresh() }
          })
      }
    }

    private func refresh() {
      guard eventMonitor != nil else { return }
      isPressed.wrappedValue = NSApp.isActive && NSEvent.modifierFlags.contains(.option)
    }

    func stop() {
      if let eventMonitor { NSEvent.removeMonitor(eventMonitor) }
      eventMonitor = nil
      notificationObservers.forEach { NotificationCenter.default.removeObserver($0) }
      notificationObservers.removeAll()
    }
  }
}
