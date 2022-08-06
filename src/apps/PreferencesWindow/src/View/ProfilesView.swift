import SwiftUI

struct ProfilesView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @State private var showingSheet = false
  @State private var editingProfile: LibKrbn.Profile?

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      List {
        VStack(alignment: .leading, spacing: 0.0) {
          ForEach($settings.profiles) { $profile in
            HStack(alignment: .center, spacing: 0) {
              Button(action: {
                settings.selectProfile(profile)
              }) {
                HStack {
                  HStack {
                    if profile.selected {
                      Image(systemName: "checkmark.circle.fill")
                    } else {
                      Image(systemName: "circle")
                    }
                  }
                  .foregroundColor(.accentColor)

                  Text(profile.name)
                }
              }
              .buttonStyle(.plain)

              Spacer()

              Button(action: {
                editingProfile = profile
                showingSheet = true
              }) {
                Label("Edit", systemImage: "pencil.circle.fill")
              }

              HStack {
                Spacer()

                if !profile.selected {
                  Button(action: {
                    settings.removeProfile(profile)
                  }) {
                    Image(systemName: "trash.fill")
                      .buttonLabelStyle()
                  }
                  .deleteButtonStyle()
                }
              }
              .frame(width: 60)
            }
            .padding(.vertical, 12.0)

            Divider()
          }

          Button(action: {
            settings.appendProfile()
          }) {
            Label("Add new profile", systemImage: "plus.circle.fill")
          }
          .padding(.top, 20.0)

          Spacer()
        }
      }
      .background(Color(NSColor.textBackgroundColor))
    }
    .padding()
    .sheet(isPresented: $showingSheet) {
      ProfileEditView(profile: $editingProfile, showing: $showingSheet)
    }
  }
}

struct ProfilesView_Previews: PreviewProvider {
  static var previews: some View {
    ProfilesView()
  }
}
