import SwiftUI

public class AppIcon: Identifiable, Equatable {
  public var id: String
  public var karabinerElementsThumbnailImage: NSImage?
  public var eventViewerThumbnailImage: NSImage?
  public var multitouchExtensionThumbnailImage: NSImage?

  init(_ id: String) {
    self.id = id

    karabinerElementsThumbnailImage = NSImage(named: "\(id)-KarabinerElements.png")
    eventViewerThumbnailImage = NSImage(named: "\(id)-EventViewer.png")
    multitouchExtensionThumbnailImage = NSImage(named: "\(id)-MultitouchExtension.png")
  }

  public static func == (lhs: AppIcon, rhs: AppIcon) -> Bool {
    lhs.id == rhs.id
  }
}

public class AppIcons: ObservableObject {
  public static let shared = AppIcons()

  @Published var icons: [AppIcon] = []

  init() {
    icons.append(AppIcon("000"))
    icons.append(AppIcon("001"))
  }
}
