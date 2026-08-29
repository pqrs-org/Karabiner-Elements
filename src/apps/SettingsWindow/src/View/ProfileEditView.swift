import SwiftUI

struct ProfileEditView: View {
  @Binding var profile: SettingsConfiguration.Profile?
  @Binding var showing: Bool
  let onEditingCancelledByExternalChange: () -> Void
  @State private var name = ""
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
    }
    .onChange(of: monitoredProfiles) { _ in
      cancelEditingIfProfileChangedExternally()
    }
  }

  private func save() {
    guard profileIsCurrent() else {
      cancelEditingIfProfileChangedExternally()
      return
    }

    settings.updateProfileName(profile!, name)
    showing = false
  }

  private var monitoredProfiles: [MonitoredProfile] {
    settings.configuration.profiles.map { MonitoredProfile($0) }
  }

  // A profile index may point to a different profile after karabiner.json is edited directly.
  // Compare the profile captured when this editor opened with the current profile at that index so
  // the editor can be dismissed before renaming a different profile. A selection change is also
  // detected when it affects the profile being edited.
  private func profileIsCurrent() -> Bool {
    guard
      let editedProfile = profile,
      settings.configuration.profiles.indices.contains(editedProfile.index)
    else {
      return false
    }

    let currentProfile = settings.configuration.profiles[editedProfile.index]
    return currentProfile.index == editedProfile.index
      && currentProfile.name == editedProfile.name
      && currentProfile.selected == editedProfile.selected
  }

  private func cancelEditingIfProfileChangedExternally() {
    guard showing, !profileIsCurrent() else {
      return
    }

    showing = false
    onEditingCancelledByExternalChange()
  }
}

private struct MonitoredProfile: Equatable {
  let index: Int
  let name: String
  let selected: Bool

  init(_ profile: SettingsConfiguration.Profile) {
    index = profile.index
    name = profile.name
    selected = profile.selected
  }
}
