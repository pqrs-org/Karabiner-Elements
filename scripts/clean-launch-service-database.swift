import AppKit

let bundleIdentifiers = [
  // Karabiner-Elements
  "org.pqrs.Karabiner-EventViewer",
  "org.pqrs.Karabiner-Menu",
  "org.pqrs.Karabiner-MultitouchExtension",
  "org.pqrs.Karabiner-NotificationWindow",
  "org.pqrs.Karabiner-Elements.Settings",

  // Karabiner-DriverKit-VirtualHIDDevice
  "org.pqrs.HIDManagerTool",
  "org.pqrs.Karabiner-DriverKit-VirtualHIDDeviceClient",
  "org.pqrs.Karabiner-VirtualHIDDevice-Manager",
  "org.pqrs.virtual-hid-device-service-client",
]

let lsregisterCommandPath =
  "/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/LaunchServices.framework/Versions/A/Support/lsregister"
let deleteTargetURLRegex = #/\/build\/|\/Build\//#

for bundleIdentifier in bundleIdentifiers {
  let urls = NSWorkspace.shared.urlsForApplications(withBundleIdentifier: bundleIdentifier)

  for url in urls {
    let path = url.path
    if path.matches(of: deleteTargetURLRegex).count > 0 {
      print("unregister \(path)")

      let process = Process()
      process.launchPath = lsregisterCommandPath
      process.arguments = [
        "-u",
        path,
      ]

      process.launch()
      process.waitUntilExit()
    }
  }
}
