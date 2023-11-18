import AppKit

//
// The built application is automatically registered in the Launch Services database.
// This means that unwanted entries such as */build/Release/*.app will be registered.
//
// Then, the names displayed in Background items of Login Items System Settings refer to the launch services database.
// If the path of the built application is referenced, the application name may not be taken properly and the developer name may appear in Login Items.
//
// To avoid this problem, unintended application entries should be removed from the database.
// This script removes such entries which includes /Build/ or /build/ in the file path.
//

if #available(macOS 13.0, *) {
  let lsregisterCommandPath =
    "/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/LaunchServices.framework/Versions/A/Support/lsregister"
  let deleteTargetPathRegex = #/
  /build/ |
  /Build/
/#

  for bundleIdentifier in bundleIdentifiers {
    print("clean-launch-services-database \(bundleIdentifier)...")

    let urls = NSWorkspace.shared.urlsForApplications(withBundleIdentifier: bundleIdentifier)

    for url in urls {
      let path = url.path
      if path.matches(of: deleteTargetPathRegex).count > 0 {
        print("unregister from the Launch Services database: \(path)")

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
}
