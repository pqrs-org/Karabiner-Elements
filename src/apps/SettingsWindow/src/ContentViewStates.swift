import SwiftUI

@MainActor
final class ContentViewStates: ObservableObject {
  static let shared = ContentViewStates()

  //
  // Alerts
  //

  @Published public private(set) var currentAlert: SettingsWindowGuidanceAlert = .none {
    didSet {
      resetDismissedAlertIfNeeded()
    }
  }
  @Published public private(set) var currentSetup: SettingsWindowGuidanceSetup = .none
  @Published private var dismissedAlert: SettingsWindowGuidanceAlert = .none
  @Published var guidanceContext = SettingsWindowGuidanceContext()
  @Published var coreServiceDaemonState = SettingsWindowCoreServiceState()
  @Published private var consoleUserServerClientConnected = false {
    didSet {
      resetDismissedAlertIfNeeded()
    }
  }
  @Published private(set) var consoleUserServerClientWaitingSeconds = 0
  private var lastAutoPresentedSetup: SettingsWindowGuidanceSetup = .none
  private var autoOpenedSetup = false

  private var currentResolvedAlert: SettingsWindowGuidanceAlert {
    if !consoleUserServerClientConnected {
      return .consoleUserServerNotConnected
    }

    return currentAlert
  }

  var displayedAlert: SettingsWindowGuidanceAlert {
    let resolvedAlert = currentResolvedAlert
    return resolvedAlert == dismissedAlert ? .none : resolvedAlert
  }

  func updateGuidanceState(_ state: SettingsWindowGuidanceState) {
    currentAlert = state.currentAlert
    currentSetup = state.currentSetup
    guidanceContext = state.guidanceContext
    coreServiceDaemonState = state.coreServiceDaemonState

    if let item = SetupItem.from(setup: state.currentSetup) {
      if state.currentSetup != lastAutoPresentedSetup {
        lastAutoPresentedSetup = state.currentSetup
        autoOpenedSetup = true
        setupSelection = item
        navigationSelection = .setup
      }
    } else {
      lastAutoPresentedSetup = .none
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
      return guidanceContext.servicesEnabled
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
