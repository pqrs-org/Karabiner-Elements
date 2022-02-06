import SwiftUI

final class ContentViewStates: ObservableObject {
  static let shared = ContentViewStates()

  @Published var navigationSelection: String? = NavigationTag.simpleModifications.rawValue

  //
  // ComplexModifications
  //

  @Published var complexModificationsSheetPresented: Bool = false
  @Published var complexModificationsSheetView: ComplexModificationsSheetView?
}
