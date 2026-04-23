import SwiftUI

enum SetupItem: String, CaseIterable, Identifiable, Hashable {
  case services
  case accessibility
  case inputMonitoring
  case driverExtension

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
    case .driverExtension: return "Driver Extension"
    }
  }

  static func from(alert: SettingsWindowAlert) -> SetupItem? {
    switch alert {
    case .servicesDisabled:
      .services
    case .accessibility:
      .accessibility
    case .inputMonitoringPermissions:
      .inputMonitoring
    case .driverNotActivated:
      .driverExtension
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
          if contentViewStates.setupItemCompleted(selectedItem) {
            setupCompletedMessageView(selectedItem)
          } else {
            switch selectedItem {
            case .services:
              SetupServicesView()
            case .accessibility:
              SetupAccessibilityView()
            case .inputMonitoring:
              if contentViewStates.setupItemCompleted(.accessibility) {
                SetupInputMonitoringView()
              } else {
                setupAccessibilityFirstView()
              }
            case .driverExtension:
              if #available(macOS 15.0, *) {
                SetupDriverExtensionView()
              } else {
                SetupDriverExtensionViewMacOS14()
              }
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
    contentViewStates.setupItemCompleted(item) ? "checkmark.circle.fill" : "circle"
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
      return "Background services are enabled."
    case .accessibility:
      return "Accessibility access is allowed."
    case .inputMonitoring:
      return """
        Input event capture is allowed.
        (It may be granted via Accessibility permission.)
        """
    case .driverExtension:
      return "Driver Extension is allowed."
    }
  }

  @ViewBuilder
  private func setupAccessibilityFirstView() -> some View {
    VStack(alignment: .leading, spacing: 20.0) {
      Label(
        "Please configure Accessibility first",
        systemImage: "lightbulb"
      )
      .font(.system(size: 24))

      Button {
        selectedItem = .accessibility
      } label: {
        Label(
          "Go to Accessibility Setup",
          systemImage: "arrow.left.circle.fill"
        )
      }
      .buttonStyle(.link)
    }
  }
}
