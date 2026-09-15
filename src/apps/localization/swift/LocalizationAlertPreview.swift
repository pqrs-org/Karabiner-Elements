import SwiftUI

private struct IsLocalizationPreviewKey: EnvironmentKey {
  static let defaultValue = false
}

extension EnvironmentValues {
  var isLocalizationPreview: Bool {
    get { self[IsLocalizationPreviewKey.self] }
    set { self[IsLocalizationPreviewKey.self] = newValue }
  }
}

// Preserve the real alert's enabled appearance while preventing preview actions.
struct LocalizationPreviewInteraction: ViewModifier {
  let dismiss: () -> Void

  func body(content: Content) -> some View {
    content
      .environment(\.isLocalizationPreview, true)
      .allowsHitTesting(false)
      .overlay {
        Color.clear
          .contentShape(Rectangle())
          .onTapGesture(perform: dismiss)
      }
      .background(PreviewKeyboardObserver(dismiss: dismiss).frame(width: 0, height: 0))
  }
}

// Focused alert buttons must not perform actions via Return or Space either.
private struct PreviewKeyboardObserver: NSViewRepresentable {
  let dismiss: () -> Void

  func makeCoordinator() -> Coordinator { Coordinator(dismiss: dismiss) }

  func makeNSView(context: Context) -> NSView {
    let view = NSView(frame: .zero)
    context.coordinator.start(view: view)
    return view
  }

  func updateNSView(_ view: NSView, context: Context) {
    context.coordinator.dismiss = dismiss
  }

  static func dismantleNSView(_ view: NSView, coordinator: Coordinator) {
    coordinator.stop()
  }

  @MainActor
  final class Coordinator {
    var dismiss: () -> Void
    private var monitor: Any?

    init(dismiss: @escaping () -> Void) { self.dismiss = dismiss }

    func start(view: NSView) {
      guard monitor == nil else { return }
      monitor = NSEvent.addLocalMonitorForEvents(matching: [.keyDown, .keyUp]) {
        [weak self, weak view] event in
        let intercepted = MainActor.assumeIsolated {
          guard let self, let window = view?.window, event.window === window else {
            return false
          }
          if event.type == .keyDown && event.keyCode == 53 {
            self.dismiss()
          }
          return true
        }
        return intercepted ? nil : event
      }
    }

    func stop() {
      if let monitor { NSEvent.removeMonitor(monitor) }
      monitor = nil
    }
  }
}
