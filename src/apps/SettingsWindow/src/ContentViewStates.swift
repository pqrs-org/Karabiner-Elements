import SwiftUI

@MainActor
final class ContentViewStates: ObservableObject {
  static let shared = ContentViewStates()

  //
  // Alerts
  //

  @Published public private(set) var currentAlert: SettingsWindowAlert = .none {
    didSet {
      if currentAlert != dismissedAlert {
        dismissedAlert = .none
      }
    }
  }
  @Published private var dismissedAlert: SettingsWindowAlert = .none
  @Published var alertContext = SettingsWindowAlertContext()

  var displayedAlert: SettingsWindowAlert {
    currentAlert == dismissedAlert ? .none : currentAlert
  }

  func updateAlertState(_ state: SettingsWindowAlertState) {
    currentAlert = state.currentAlert
    alertContext = state.alertContext
  }

  func dismissCurrentAlert() {
    dismissedAlert = currentAlert
  }

  //
  // ContentMainView
  //

  @Published var navigationSelection = SidebarItem.simpleModifications

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
