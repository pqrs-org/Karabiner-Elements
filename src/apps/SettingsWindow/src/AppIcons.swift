import SwiftUI

@MainActor
public class AppIcon: Identifiable, Equatable {
  nonisolated public let id: Int32
  public var karabinerElementsThumbnailImage: NSImage?
  public var eventViewerThumbnailImage: NSImage?
  public var multitouchExtensionThumbnailImage: NSImage?

  init(_ id: Int32) {
    self.id = id

    karabinerElementsThumbnailImage = NSImage(
      named: String(format: "%03d-KarabinerElements.png", id))
    eventViewerThumbnailImage = NSImage(
      named: String(format: "%03d-EventViewer.png", id))
    multitouchExtensionThumbnailImage = NSImage(
      named: String(format: "%03d-MultitouchExtension.png", id))
  }

  nonisolated public static func == (lhs: AppIcon, rhs: AppIcon) -> Bool {
    lhs.id == rhs.id
  }
}

@MainActor
public class AppIcons: ObservableObject {
  public static let shared = AppIcons()

  private var didSetEnabled = false
  // Start CoreServiceClient when init
  private var coreServiceClient = LibKrbn.CoreServiceClient.shared

  @Published var icons: [AppIcon] = []

  @Published var selectedAppIconNumber: Int32 = 0 {
    didSet {
      if didSetEnabled {
        coreServiceClient.setAppIcon(selectedAppIconNumber)
      }
    }
  }

  init() {
    icons.append(AppIcon(0))
    icons.append(AppIcon(1))
    icons.append(AppIcon(2))

    selectedAppIconNumber = librkbn_get_app_icon_number()

    didSetEnabled = true
  }
}
