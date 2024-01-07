import SwiftUI

struct ProfilesView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @State private var moveDisabled: Bool = true
  @State private var showingSheet = false
  @State private var editingProfile: LibKrbn.Profile?

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      HStack {
        Button(
          action: {
            settings.appendProfile()
          },
          label: {
            Label("Add new profile", systemImage: "plus.circle.fill")
          }
        )

        Spacer()

        if settings.profiles.count > 1 {
          HStack {
            Text("You can reorder list by dragging")
            Image(systemName: "arrow.up.arrow.down.square.fill")
              .resizable(resizingMode: .stretch)
              .frame(width: 16.0, height: 16.0)
            Text("icon")
          }
        }
      }

      List {
        ForEach($settings.profiles) { $profile in
          HStack(alignment: .center, spacing: 12.0) {
            if settings.profiles.count > 1 {
              Image(systemName: "arrow.up.arrow.down.square.fill")
                .resizable(resizingMode: .stretch)
                .frame(width: 16.0, height: 16.0)
                .onHover { hovering in
                  moveDisabled = !hovering
                }
            }

            Button(
              action: {
                settings.selectProfile(profile)
              },
              label: {
                HStack {
                  HStack {
                    if profile.selected {
                      Image(systemName: "circle.circle.fill")
                    } else {
                      Image(systemName: "circle")
                    }
                  }
                  .foregroundColor(.accentColor)

                  Text(profile.name)
                }
              }
            )
            .buttonStyle(.plain)

            Spacer()

            Button(
              action: {
                editingProfile = profile
                showingSheet = true
              },
              label: {
                Label("Rename", systemImage: "pencil.circle.fill")
              })

            Button(
              action: {
                settings.duplicateProfile(profile)
              },
              label: {
                Label("Duplicate", systemImage: "person.2.fill")
              })

            HStack {
              Spacer()

              if !profile.selected {
                Button(
                  action: {
                    settings.removeProfile(profile)
                  },
                  label: {
                    Image(systemName: "trash.fill")
                      .buttonLabelStyle()
                  }
                )
                .deleteButtonStyle()
              }
            }
            .frame(width: 60)
          }
          .padding(.vertical, 5.0)
          .moveDisabled(moveDisabled)
        }
        .onMove { indices, destination in
          if let first = indices.first {
            settings.moveProfile(first, destination)
          }
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
