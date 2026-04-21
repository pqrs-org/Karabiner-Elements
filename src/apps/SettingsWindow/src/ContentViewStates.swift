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
  private var autoOpenedSetup = false

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

    if let item = SetupItem.from(alert: state.currentAlert) {
      if state.currentAlert != lastAutoPresentedSetupAlert {
        lastAutoPresentedSetupAlert = state.currentAlert
        autoOpenedSetup = true
        setupSelection = item
        navigationSelection = .setup
      }
    } else {
      lastAutoPresentedSetupAlert = .none
      if autoOpenedSetup && navigationSelection == .setup && allSetupItemsCompleted() {
        autoOpenedSetup = false
        navigationSelection = .simpleModifications
      }
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

  func userSelectedNavigationItem(_ item: SidebarItem) {
    autoOpenedSetup = false

    if navigationSelection != item {
      navigationSelection = item
    }
  }

  func userSelectedSetupItem(_ item: SetupItem) {
    if setupSelection != item {
      setupSelection = item
    }
  }

  func allSetupItemsCompleted() -> Bool {
    SetupItem.allCases.allSatisfy { setupItemCompleted($0) }
  }

  func setupItemCompleted(_ item: SetupItem) -> Bool {
    switch item {
    case .services:
      return alertContext.servicesEnabled && alertContext.coreDaemonsRunning
        && alertContext.coreAgentsRunning
    case .accessibility:
      return coreServiceDaemonState.accessibilityProcessTrusted == true
    case .inputMonitoring:
      return coreServiceDaemonState.inputMonitoringGranted == true
    case .driverExtension:
      return coreServiceDaemonState.driverActivated == true
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
