import AppKit

@MainActor
final class KarabinerAppHelper {
  public static let shared = KarabinerAppHelper()

  func openMultitouchExtensionSettings() {
    guard
      let applicationURL = NSWorkspace.shared.urlForApplication(
        withBundleIdentifier: "org.pqrs.Karabiner-MultitouchExtension")
    else { return }

    NSWorkspace.shared.openApplication(
      at: applicationURL,
      configuration: NSWorkspace.OpenConfiguration())
  }
}
