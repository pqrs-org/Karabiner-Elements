import SwiftUI

// Use AppKit's native text rendering: SwiftUI's toolbar menu Picker can render
// the selected text white on a light background on macOS 27.
@MainActor
struct LanguagePicker: NSViewRepresentable {
  @AppLocalizationContext private var localized
  @Binding var selection: String
  let languages: [String]

  func makeCoordinator() -> Coordinator {
    Coordinator(selection: $selection)
  }

  func makeNSView(context: Context) -> NSPopUpButton {
    let button = NSPopUpButton(frame: .zero, pullsDown: false)
    button.target = context.coordinator
    button.action = #selector(Coordinator.selectionChanged(_:))
    button.imagePosition = .imageLeft
    button.setContentCompressionResistancePriority(.required, for: .horizontal)
    return button
  }

  func updateNSView(_ button: NSPopUpButton, context: Context) {
    context.coordinator.selection = $selection
    let identifiers = ["auto"] + languages
    let titles = identifiers.map {
      $0 == "auto" ? localized("shared.language.auto") : AppLanguage.displayName(for: $0)
    }
    if button.itemArray.map({ $0.representedObject as? String }) != identifiers.map(Optional.some)
      || button.itemTitles != titles
    {
      button.removeAllItems()
      for (identifier, title) in zip(identifiers, titles) {
        let item = NSMenuItem(title: title, action: nil, keyEquivalent: "")
        item.representedObject = identifier
        item.image = NSImage(systemSymbolName: "globe", accessibilityDescription: nil)
        button.menu?.addItem(item)
      }
    }
    button.selectItem(at: identifiers.firstIndex(of: selection) ?? 0)
    button.setAccessibilityLabel(localized("shared.language.title"))
    button.invalidateIntrinsicContentSize()
  }

  func sizeThatFits(_ proposal: ProposedViewSize, nsView: NSPopUpButton, context: Context)
    -> CGSize?
  {
    nsView.intrinsicContentSize
  }

  @MainActor
  final class Coordinator: NSObject {
    var selection: Binding<String>

    init(selection: Binding<String>) {
      self.selection = selection
    }

    @objc func selectionChanged(_ sender: NSPopUpButton) {
      guard let language = sender.selectedItem?.representedObject as? String else { return }
      selection.wrappedValue = language
    }
  }
}
