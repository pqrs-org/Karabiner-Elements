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

  // This value is maintained by Settings locally, outside of
  // SettingsWindowGuidanceState from console_user_server.
  @Published public private(set) var localSetup: SettingsWindowGuidanceSetup = .none
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

  // These values are maintained by Settings locally, outside of
  // SettingsWindowGuidanceState from console_user_server.
  private var localCoreDaemonsEnabled = true
  private var localCoreAgentsEnabled = true
  private var localServicesSetupPresented = false

  private var currentResolvedAlert: SettingsWindowGuidanceAlert {
    // When Settings cannot connect to console_user_server, it cannot fetch
    // SettingsWindowGuidanceState from console_user_server, so this error handling
    // has to run independently from SettingsWindowGuidanceState.
    if !consoleUserServerClientConnected {
      // There are multiple reasons why Settings might not be able to connect to
      // console_user_server.
      //
      // 1. org.pqrs.service.daemon.Karabiner-Core-Service is not enabled, so
      //    /Library/Application Support/org.pqrs/tmp has not been created yet and
      //    console_user_server cannot bind its socket.
      // 2. org.pqrs.service.agent.karabiner_console_user_server is disabled and does
      //    not start.
      // 3. console_user_server is still starting up and has not bound its socket yet.
      //
      // Cases 1 and 2 are incomplete setup states. Case 1 always happens just after
      // installation. Treat them as setup states rather than errors, and open
      // SetupServicesView.
      //
      if localServicesRequireAttention {
        return .none
      }

      // In the other cases, show ConsoleUserServerNotConnectedAlertView.
      return .consoleUserServerNotConnected
    }

    return currentAlert
  }

  private var localServicesRequireAttention: Bool {
    localCoreDaemonsEnabled == false || localCoreAgentsEnabled == false
  }

  var displayedAlert: SettingsWindowGuidanceAlert {
    let resolvedAlert = currentResolvedAlert
    return resolvedAlert == dismissedAlert ? .none : resolvedAlert
  }

  var currentResolvedSetup: SettingsWindowGuidanceSetup {
    localSetup == .none ? currentSetup : localSetup
  }

  func updateGuidanceState(_ state: SettingsWindowGuidanceState) {
    if currentAlert != state.currentAlert {
      currentAlert = state.currentAlert
    }
    if currentSetup != state.currentSetup {
      currentSetup = state.currentSetup
    }
    if guidanceContext != state.guidanceContext {
      guidanceContext = state.guidanceContext
    }
    if coreServiceDaemonState != state.coreServiceDaemonState {
      coreServiceDaemonState = state.coreServiceDaemonState
    }

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

  func updateLocalServicesGuidanceContext(
    coreDaemonsEnabled: Bool,
    coreAgentsEnabled: Bool
  ) {
    localCoreDaemonsEnabled = coreDaemonsEnabled
    localCoreAgentsEnabled = coreAgentsEnabled

    if !consoleUserServerClientConnected && localServicesRequireAttention {
      if localSetup != .services {
        localSetup = .services
      }

      if !localServicesSetupPresented {
        localServicesSetupPresented = true
        setupSelection = .services
        navigationSelection = .setup
      }
    } else if localServicesSetupPresented {
      localServicesSetupPresented = false

      if localSetup == .services {
        localSetup = .none
      }
    }
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
      return guidanceContext.servicesEnabled == true
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
