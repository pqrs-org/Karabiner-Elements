import SwiftUI

struct ProfileEditView: View {
  @Binding var profile: LibKrbn.Profile?
  @Binding var showing: Bool
  @State private var name = ""
  @ObservedObject private var settings = LibKrbn.Settings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      if profile != nil {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack {
            Text("Profile name:")
            TextField("Profile name", text: $name)
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
                settings.updateProfileName(profile!, name)
                showing = false
              },
              label: {
                Label("Save", systemImage: "checkmark")
                  .buttonLabelStyle()
              }
            )
            .buttonStyle(BorderedProminentButtonStyle())
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
  }
}
