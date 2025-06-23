import SwiftUI

@MainActor
final class ContentViewStates: ObservableObject {
  static let shared = ContentViewStates()

  //
  // Alerts
  //

  @Published public var showServicesNotRunningAlert = false
  @Published public var showDriverNotActivatedAlert = false
  @Published public var showDriverVersionMismatchedAlert = false
  @Published public var showInputMonitoringPermissionsAlert = false
  @Published public var showDoctorAlert = false
  @Published public var showSettingsAlert = false

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
