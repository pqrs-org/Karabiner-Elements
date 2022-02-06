import SwiftUI

final class ContentViewStates: ObservableObject {
  static let shared = ContentViewStates()

  @Published var navigationSelection: String? = NavigationTag.simpleModifications.rawValue
}
