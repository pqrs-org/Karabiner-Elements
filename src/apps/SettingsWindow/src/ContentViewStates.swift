import SwiftUI

@MainActor
final class ContentViewStates: ObservableObject {
  static let shared = ContentViewStates()

  //
  // Alerts
  //

  @Published public private(set) var currentAlert: SettingsWindowAlert = .none {
    didSet {
      resetDismissedAlertIfNeeded()
    }
  }
  @Published private var dismissedAlert: SettingsWindowAlert = .none
  @Published var alertContext = SettingsWindowAlertContext()
  @Published private var consoleUserServerClientConnected = false {
    didSet {
      resetDismissedAlertIfNeeded()
    }
  }

  private var currentResolvedAlert: SettingsWindowAlert {
    if !consoleUserServerClientConnected {
      return .consoleUserServerNotConnected
    }

    return currentAlert
  }

  var displayedAlert: SettingsWindowAlert {
    let resolvedAlert = currentResolvedAlert
    return resolvedAlert == dismissedAlert ? .none : resolvedAlert
  }

  func updateAlertState(_ state: SettingsWindowAlertState) {
    currentAlert = state.currentAlert
    alertContext = state.alertContext
  }

  func updateConsoleUserServerClientConnected(_ connected: Bool) {
    consoleUserServerClientConnected = connected
  }

  func dismissCurrentAlert() {
    dismissedAlert = currentResolvedAlert
  }

  private func resetDismissedAlertIfNeeded() {
    if currentResolvedAlert != dismissedAlert {
      dismissedAlert = .none
    }
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
