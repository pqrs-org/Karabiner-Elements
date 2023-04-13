import AppKit

for argument in CommandLine.arguments {

  if let icon = NSImage(named: "\(argument)-KarabinerElements.icns") {
    let result = NSWorkspace.shared.setIcon(
      icon,
      forFile: "/Applications/Karabiner-Elements.app",
      options: [])
    print("setIcon Karabiner-Elements.app: \(result)")
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
