import SwiftUI

struct ProfilesView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @State private var moveDisabled: Bool = true
  @State private var showingSheet = false
  @State private var hoverProfile: LibKrbn.Profile?
  @State private var editingProfile: LibKrbn.Profile?

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      HStack {
        Button(
          action: {
            settings.appendProfile()
          },
          label: {
            AccentColorIconLabel(title: "Add new profile", systemImage: "plus.circle.fill")
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
          // Make a copy to use it in onHover.
          // (Without copy, the program crashes with an incorrect reference when the profile is deleted.)
          let profileCopy = profile

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
                HStack(spacing: 0.0) {
                  HStack(spacing: 0.0) {
                    if profile.selected {
                      Image(systemName: "circle.circle.fill")
                    } else {
                      Image(systemName: "circle")
                    }
                  }
                  .foregroundColor(.accentColor)

                  Text(profile.name)
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(.horizontal, 6.0)
                    .padding(.vertical, 2.0)
                    .if(hoverProfile == profile) {
                      $0.overlay(
                        RoundedRectangle(cornerRadius: 2)
                          .stroke(
                            Color(NSColor(Color.accentColor)),
                            lineWidth: 1
                          )
                      )
                    }
                }
              }
            )
            .buttonStyle(.plain)

            HStack(alignment: .center, spacing: 10) {
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
                if !profile.selected {
                  Button(
                    role: .destructive,
                    action: {
                      settings.removeProfile(profile)
                    },
                    label: {
                      Image(systemName: "trash")
                        .buttonLabelStyle()
                    }
                  )
                  .deleteButtonStyle()
                }
              }
              .frame(width: 60)
            }
            .onHover { hovering in
              if hovering {
                hoverProfile = profileCopy
              } else {
                if hoverProfile == profileCopy {
                  hoverProfile = nil
                }
              }
            }
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
