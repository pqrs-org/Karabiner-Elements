import AppKit
import SwiftUI

@MainActor
struct SearchField: NSViewRepresentable {
  @Binding var text: String
  var placeholderString = "Filter"
  var debounceInterval: TimeInterval = 0.2

  func makeCoordinator() -> Coordinator {
    Coordinator(text: $text, debounceInterval: debounceInterval)
  }

  func makeNSView(context: Context) -> NSSearchField {
    let searchField = NSSearchField()
    searchField.placeholderString = placeholderString
    searchField.delegate = context.coordinator
    context.coordinator.installKeyDownMonitor(for: searchField)
    return searchField
  }

  static func dismantleNSView(_ searchField: NSSearchField, coordinator: Coordinator) {
    coordinator.removeKeyDownMonitor()
  }

  func updateNSView(_ searchField: NSSearchField, context: Context) {
    context.coordinator.update(text: $text, debounceInterval: debounceInterval)
    searchField.placeholderString = placeholderString
    if !context.coordinator.isDebouncing && searchField.stringValue != text {
      searchField.stringValue = text
    }
  }

  final class Coordinator: NSObject, NSSearchFieldDelegate {
    @Binding private var text: String
    private var debounceInterval: TimeInterval
    private var timer: Timer?
    private var pendingText = ""
    private var keyDownMonitor: Any?

    var isDebouncing: Bool {
      timer != nil
    }

    init(text: Binding<String>, debounceInterval: TimeInterval) {
      _text = text
      self.debounceInterval = debounceInterval
    }

    func update(text: Binding<String>, debounceInterval: TimeInterval) {
      _text = text
      self.debounceInterval = debounceInterval
    }

    // Set up keyboard shortcuts to focus the search field with command+f or /.
    func installKeyDownMonitor(for searchField: NSSearchField) {
      keyDownMonitor = NSEvent.addLocalMonitorForEvents(
        matching: .keyDown
      ) { [weak searchField] event in
        guard let searchField else { return event }

        let eventWindow = event.window
        let modifiers = event.modifierFlags.intersection([.command, .option, .control, .shift])
        let characters = event.charactersIgnoringModifiers

        // Local event monitors are invoked on the main thread, although the API's handler is not
        // annotated with @MainActor.
        let handled = MainActor.assumeIsolated {
          guard eventWindow === searchField.window else { return false }

          let commandF = modifiers == .command && characters == "f"
          let isEditingText =
            (eventWindow?.firstResponder as? NSTextView)?.isEditable == true
          let slash =
            modifiers.isEmpty
            && characters == "/"
            && !isEditingText

          if commandF || slash {
            eventWindow?.makeFirstResponder(searchField)
            return true
          }

          return false
        }

        return handled ? nil : event
      }
    }

    func removeKeyDownMonitor() {
      if let keyDownMonitor {
        NSEvent.removeMonitor(keyDownMonitor)
        self.keyDownMonitor = nil
      }
    }

    func controlTextDidChange(_ notification: Notification) {
      guard let searchField = notification.object as? NSSearchField else { return }

      timer?.invalidate()

      if debounceInterval <= 0 {
        text = searchField.stringValue
        timer = nil
      } else {
        pendingText = searchField.stringValue
        timer = Timer.scheduledTimer(
          timeInterval: debounceInterval,
          target: self,
          selector: #selector(commitPendingText),
          userInfo: nil,
          repeats: false)
      }
    }

    @objc private func commitPendingText() {
      text = pendingText
      timer = nil
    }

    deinit {
      timer?.invalidate()
    }
  }
}
