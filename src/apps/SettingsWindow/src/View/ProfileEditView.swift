import SwiftUI

struct ProfileEditView: View {
  @Binding var profile: SettingsConfiguration.Profile?
  @Binding var showing: Bool
  @State private var name = ""
  @State private var expectedConfigurationGeneration: UInt64?
  @State private var errorMessage: String?
  @ObservedObject private var settings = Settings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      if profile != nil {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack {
            Text("Profile name:")
            TextField("Profile name", text: $name)
              .onSubmit {
                save()
              }
          }

          if let errorMessage {
            Label(errorMessage, systemImage: ErrorBorder.icon)
              .modifier(ErrorBorder())
          }

          HStack(alignment: .center) {
            Button(
              action: {
                showing = false
              },
              label: {
                Label("Cancel", systemImage: "xmark")
              })

            Button(
              action: {
                save()
              },
              label: {
                Label("Save", systemImage: "checkmark")
                  .buttonLabelStyle()
              }
            )
            .buttonStyle(BorderedProminentButtonStyle())
            .keyboardShortcut("s")
            .padding(.leading, 24.0)
          }
          .frame(maxWidth: .infinity, alignment: .center)
        }
        .padding()
      }
    }
    .padding()
    .frame(width: 400)
    .onAppear {
      name = profile?.name ?? ""
      expectedConfigurationGeneration = settings.configurationGeneration
    }
  }

  private func save() {
    guard expectedConfigurationGeneration == settings.configurationGeneration else {
      errorMessage = "The configuration has changed. Close this editor and try again."
      return
    }

    settings.updateProfileName(profile!, name)
    showing = false
  }
}
