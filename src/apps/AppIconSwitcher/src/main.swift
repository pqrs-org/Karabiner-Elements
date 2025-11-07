import AppKit

let appIconNumber = String(format: "%03d", librkbn_get_app_icon_number())

for file in [
  "/Applications/Karabiner-Elements.app",
  "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Core-Service.app",
  "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Elements Non-Privileged Agents v2.app",
  "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Elements Privileged Daemons v2.app",
  "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Menu.app",
  "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-NotificationWindow.app",
  "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Updater.app",
  "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-Updater.app/Contents/Frameworks/Sparkle.framework/Updater.app",
] {
  let iconName = "\(appIconNumber)-KarabinerElements.icns"
  setIcon(iconName: iconName, file: String(file))
}

setIcon(
  iconName: "\(appIconNumber)-EventViewer.icns",
  file: String("/Applications/Karabiner-EventViewer.app")
)

setIcon(
  iconName: "\(appIconNumber)-MultitouchExtension.icns",
  file: String(
    "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-MultitouchExtension.app"
  )
)

private func setIcon(iconName: String, file: String) {
  print("setIcon:")
  print("  iconName: \(iconName)")
  print("  file: \(file)")

  if let icon = NSImage(named: iconName) {
    let result = NSWorkspace.shared.setIcon(
      icon,
      forFile: file,
      options: [])
    print("  result \(result)")
  }
}
