import AppKit
import SwiftUI

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
    return searchField
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
