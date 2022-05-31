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
              Text(profile.name)

              Button(action: {
                editingProfile = profile
                showingSheet = true
              }) {
                Label("Edit", systemImage: "pencil.circle.fill")
              }
              .padding(.leading, 12.0)

              if !profile.selected {
                Button(action: {
                  settings.removeProfile(profile)
                }) {
                  Image(systemName: "trash.fill")
                    .buttonLabelStyle()
                }
                .deleteButtonStyle()
                .padding(.leading, 12.0)
              }

              Spacer()

              if profile.selected {
                Label("Selected", systemImage: "checkmark.square.fill")
              } else {
                Button(action: {
                  settings.selectProfile(profile)
                }) {
                  Label("Select", systemImage: "square")
                }
              }
            }
            .padding(12.0)
            .overlay(
              RoundedRectangle(cornerRadius: 8)
                .stroke(
                  Color(NSColor.selectedControlColor),
                  lineWidth: profile.selected ? 3 : 0
                )
                .padding(2)
            )

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
