import AppKit
import SwiftUI

struct FindCommands: Commands {
  var body: some Commands {
    CommandGroup(replacing: .textEditing) {
      Button("Find…") {
        FindCommand.perform(.showFindPanel)
      }
      .keyboardShortcut("f", modifiers: .command)

      Button("Find Next") {
        FindCommand.perform(.next)
      }
      .keyboardShortcut("g", modifiers: .command)

      Button("Find Previous") {
        FindCommand.perform(.previous)
      }
      .keyboardShortcut("g", modifiers: [.command, .shift])
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
