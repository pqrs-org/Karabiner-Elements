import SwiftUI

enum SetupItem: String, CaseIterable, Identifiable, Hashable {
  case services
  case accessibility

  var id: Self { self }

  var title: String {
    switch self {
    case .services: return "Background Services"
    case .accessibility: return "Accessibility"
    }
  }

  static func from(alert: SettingsWindowAlert) -> SetupItem? {
    switch alert {
    case .servicesNotRunning:
      .services
    case .accessibility:
      .accessibility
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
      return "Background services are running"
    case .accessibility:
      return "Accessibility access is allowed"
    }
  }
}
