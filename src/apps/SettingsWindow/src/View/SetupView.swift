import SwiftUI

enum SetupItem: String, CaseIterable, Identifiable, Hashable {
  case services
  case accessibility
  case inputMonitoring

  var id: Self { self }

  var title: String {
    switch self {
    case .services: return "Background Services"
    case .accessibility: return "Accessibility"
    // Depending on the macOS version, granting Accessibility permission may also allow Input Monitoring.
    // In that case, both requirements are considered completed once Accessibility is granted, so if we call it
    // "Input Monitoring", users may wonder why it is marked as completed even though they did not explicitly
    // allow Input Monitoring.
    // To avoid that confusion, the displayed label is changed to "Capture Input Events".
    case .inputMonitoring: return "Capture Input Events"
    }
  }

  static func from(alert: SettingsWindowAlert) -> SetupItem? {
    switch alert {
    case .servicesNotRunning:
      .services
    case .accessibility:
      .accessibility
    case .inputMonitoringPermissions:
      .inputMonitoring
    default:
      nil
    }
  }
}

struct SetupView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared

  @State private var selectedItem: SetupItem = .services

  var body: some View {
    HStack(spacing: 0) {
      List(SetupItem.allCases, selection: $selectedItem) { item in
        Label(
          title: {
            Text(item.title)
              .lineLimit(nil)
              .fixedSize(horizontal: false, vertical: true)
          },
          icon: {
            Image(systemName: setupStatusSystemImage(item))
              .frame(width: 20.0)
          }
        )
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(.vertical, 8)
        .listRowSeparator(.visible, edges: .bottom)
        .tag(item)
      }
      .frame(width: 250)
      .listStyle(.sidebar)

      Divider()

      ScrollView {
        Group {
          if setupItemCompleted(selectedItem) {
            setupCompletedMessageView(selectedItem)
          } else {
            switch selectedItem {
            case .services:
              SetupServicesView()
            case .accessibility:
              SetupAccessibilityView()
            case .inputMonitoring:
              SetupInputMonitoringView()
            }
          }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
        .padding()
      }
    }
    .onAppear {
      selectedItem = contentViewStates.setupSelection
    }
    .onChange(of: selectedItem) { newValue in
      contentViewStates.userSelectedSetupItem(newValue)
    }
    .onChange(of: contentViewStates.setupSelection) { newValue in
      if selectedItem != newValue {
        selectedItem = newValue
      }
    }
  }

  private func setupStatusSystemImage(_ item: SetupItem) -> String {
    setupItemCompleted(item) ? "checkmark.circle.fill" : "circle"
  }

  private func setupItemCompleted(_ item: SetupItem) -> Bool {
    switch item {
    case .services:
      let context = contentViewStates.alertContext
      return context.servicesEnabled && context.coreDaemonsRunning && context.coreAgentsRunning
    case .accessibility:
      return contentViewStates.coreServiceDaemonState.accessibilityProcessTrusted == true
    case .inputMonitoring:
      return contentViewStates.coreServiceDaemonState.inputMonitoringGranted == true
    }
  }

  @ViewBuilder
  private func setupCompletedMessageView(_ item: SetupItem) -> some View {
    VStack(alignment: .leading, spacing: 16.0) {
      Label(
        setupCompletedTitle(item),
        systemImage: "checkmark.circle.fill"
      )
    }
    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
  }

  private func setupCompletedTitle(_ item: SetupItem) -> String {
    switch item {
    case .services:
      return "Background services are running."
    case .accessibility:
      return "Accessibility access is allowed."
    case .inputMonitoring:
      return """
        Input event capture is allowed.
        (It may be granted via Accessibility permission.)
        """
    }
  }
}
