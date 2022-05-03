import SwiftUI

final class ContentViewStates: ObservableObject {
  static let shared = ContentViewStates()

  @Published var navigationSelection: String? = NavigationTag.simpleModifications.rawValue

  //
  // SimpleModificationsView
  //

    @Published var simpleModificationsViewSelectedDevice: LibKrbn.ConnectedDevice?

  //
  // FunctionKeysView
  //

    @Published var functionKeysViewSelectedDevice: LibKrbn.ConnectedDevice?

  //
  // ComplexModifications
  //

  @Published var complexModificationsViewSheetPresented: Bool = false
  @Published var complexModificationsViewSheetView: ComplexModificationsSheetView?
}
