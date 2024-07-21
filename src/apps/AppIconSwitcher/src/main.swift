import AppKit

for argument in CommandLine.arguments {
  for file in [
    "/Applications/Karabiner-Elements.app",
    "/Applications/Karabiner-Elements.app/Contents/Frameworks/Sparkle.framework/Updater.app",
    "/Library/Application Support/org.pqrs//Karabiner-Elements/Karabiner-Elements Non-Privileged Agents.app",
    "/Library/Application Support/org.pqrs//Karabiner-Elements/Karabiner-Elements Privileged Daemons.app",
    "/Library/Application Support/org.pqrs//Karabiner-Elements/Karabiner-Menu.app",
    "/Library/Application Support/org.pqrs//Karabiner-Elements/Karabiner-NotificationWindow.app",
  ] {
    if let icon = NSImage(named: "\(argument)-KarabinerElements.icns") {
      let result = NSWorkspace.shared.setIcon(
        icon,
        forFile: file,
        options: [])
      print("setIcon Karabiner-Elements.app: \(result)")
    }
  }

  if let icon = NSImage(named: "\(argument)-EventViewer.icns") {
    let result = NSWorkspace.shared.setIcon(
      icon,
      forFile: "/Applications/Karabiner-EventViewer.app",
      options: [])

    print("setIcon Karabiner-EventViewer.app: \(result)")
  }

  if let icon = NSImage(named: "\(argument)-MultitouchExtension.icns") {
    let result = NSWorkspace.shared.setIcon(
      icon,
      forFile:
        "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-MultitouchExtension.app",
      options: [])

    print("setIcon Karabiner-MultitouchExtension.app: \(result)")
  }
}
