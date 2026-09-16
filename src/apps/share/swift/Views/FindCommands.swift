import AppKit
import SwiftUI

struct FindCommands: Commands {
  @ObservedObject private var localization = AppLocalization.shared
  var locale: Locale = .autoupdatingCurrent

  private func text(_ key: String) -> String {
    AppLanguage.text(key, locale: locale, catalog: localization.catalog)
  }

  var body: some Commands {
    CommandGroup(replacing: .textEditing) {
      Menu(text("shared.find.menu")) {
        Button(text("shared.find.show")) {
          FindCommand.perform(.showFindPanel)
        }
        .keyboardShortcut("f", modifiers: .command)

        Button(text("shared.find.next")) {
          FindCommand.perform(.next)
        }
        .keyboardShortcut("g", modifiers: .command)

        Button(text("shared.find.previous")) {
          FindCommand.perform(.previous)
        }
        .keyboardShortcut("g", modifiers: [.command, .shift])
      }
    }
  }

}

@MainActor
enum FindCommand {
  static func perform(_ action: NSFindPanelAction) {
    let menuItem = NSMenuItem()
    menuItem.tag = Int(action.rawValue)

    NSApp.sendAction(
      #selector(NSTextView.performFindPanelAction(_:)),
      to: nil,
      from: menuItem)
  }
}
