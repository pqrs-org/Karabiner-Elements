import AppKit

let appIconNumber = String(format: "%03d", krbn_get_app_icon_number())

for file in [
  "/Applications/Karabiner-Elements.app",
  "/Applications/Karabiner-Elements.app/Contents/Applications/Karabiner-Core-Service.app",
  "/Applications/Karabiner-Elements.app/Contents/Applications/Karabiner-Elements Non-Privileged Agents v3.app",
  "/Applications/Karabiner-Elements.app/Contents/Applications/Karabiner-Elements Privileged Daemons v3.app",
  "/Applications/Karabiner-Elements.app/Contents/Applications/Karabiner-Console-User-Server.app",
  "/Applications/Karabiner-Elements.app/Contents/Applications/Karabiner-Updater.app",
  "/Applications/Karabiner-Elements.app/Contents/Applications/Karabiner-Updater.app/Contents/Frameworks/Sparkle.framework/Updater.app",
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
    "/Applications/Karabiner-Elements.app/Contents/Applications/Karabiner-MultitouchExtension.app"
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
