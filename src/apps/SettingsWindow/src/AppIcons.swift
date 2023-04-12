import SwiftUI

public class AppIcon: Identifiable, Equatable {
  public var id: String
  public var karabinerElementsThumbnailImageName: String
  public var eventViewerThumbnailImageName: String
  public var multitouchExtensionThumbnailImageName: String
  public var karabinerElementsIcon: NSImage?
  public var eventViewerIcon: NSImage?
  public var multitouchExtensionIcon: NSImage?

  init(_ id: String) {
    self.id = id

    karabinerElementsThumbnailImageName = "\(id)-KarabinerElements"
    eventViewerThumbnailImageName = "\(id)-EventViewer"
    multitouchExtensionThumbnailImageName = "\(id)-MultitouchExtension"

    karabinerElementsIcon = NSImage(named: "\(id)-KarabinerElements.icns")
    eventViewerIcon = NSImage(named: "\(id)-EventViewer.icns")
    multitouchExtensionIcon = NSImage(named: "\(id)-MultitouchExtension.icns")
  }

  public static func == (lhs: AppIcon, rhs: AppIcon) -> Bool {
    lhs.id == rhs.id
  }

  public func apply() {
    NSWorkspace.shared.setIcon(
      karabinerElementsIcon,
      forFile: "/Applications/Karabiner-Elements.app",
      options: [])

    NSWorkspace.shared.setIcon(
      eventViewerIcon,
      forFile: "/Applications/Karabiner-EventViewer.app",
      options: [])

    NSWorkspace.shared.setIcon(
      multitouchExtensionIcon,
      forFile:
        "/Library/Application Support/org.pqrs/Karabiner-Elements/Karabiner-MultitouchExtension.app",
      options: [])
  }
}

public class AppIcons: ObservableObject {
  public static let shared = AppIcons()

  @Published var icons: [AppIcon] = []

  init() {
    icons.append(AppIcon("000"))
    icons.append(AppIcon("001"))
  }

  public func apply() {
    let selectedAppIcon = LibKrbn.Settings.shared.appIcon
    for icon in icons {
      if icon.id == selectedAppIcon {
        icon.apply()
        return
      }
    }

    icons[0].apply()
  }
}
