import SwiftUI

public class AppIcon: Identifiable, Equatable {
  public var id: String
  public var karabinerElementsThumbnailImageName: String
  public var eventViewerThumbnailImageName: String
  public var multitouchExtensionThumbnailImageName: String

  init(_ id: String) {
    self.id = id

    karabinerElementsThumbnailImageName = "\(id)-KarabinerElements"
    eventViewerThumbnailImageName = "\(id)-EventViewer"
    multitouchExtensionThumbnailImageName = "\(id)-MultitouchExtension"
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
