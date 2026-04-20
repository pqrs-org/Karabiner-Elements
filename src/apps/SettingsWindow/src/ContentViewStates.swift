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
  @Published var coreServiceDaemonState = SettingsWindowCoreServiceState()
  @Published private var consoleUserServerClientConnected = false {
    didSet {
      resetDismissedAlertIfNeeded()
    }
  }
  @Published private(set) var consoleUserServerClientWaitingSeconds = 0
  private var lastAutoPresentedSetupAlert: SettingsWindowAlert = .none

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
    coreServiceDaemonState = state.coreServiceDaemonState

    if let item = SetupItem.from(alert: state.currentAlert),
      state.currentAlert != lastAutoPresentedSetupAlert
    {
      lastAutoPresentedSetupAlert = state.currentAlert
      setupSelection = item
      navigationSelection = .setup
    } else if SetupItem.from(alert: state.currentAlert) == nil {
      lastAutoPresentedSetupAlert = .none
    }
  }

  func updateConsoleUserServerClientConnected(_ connected: Bool) {
    consoleUserServerClientConnected = connected
  }

  func updateConsoleUserServerClientWaitingSeconds(_ seconds: Int) {
    consoleUserServerClientWaitingSeconds = seconds
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
  @Published var setupSelection = SetupItem.services

  func userSelectedSetupItem(_ item: SetupItem) {
    if setupSelection != item {
      setupSelection = item
    }
  }

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
