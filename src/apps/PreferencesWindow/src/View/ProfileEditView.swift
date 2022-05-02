import SwiftUI

struct ProfileEditView: View {
  @Binding var profile: Profile?
  @Binding var showing: Bool
  @State private var name = ""
  @ObservedObject private var settings = Settings.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      if profile != nil {
        GroupBox(label: Text("Edit")) {
          VStack(alignment: .leading, spacing: 12.0) {
            VStack(alignment: .leading, spacing: 0.0) {
              HStack {
                Text("Profile name:")
                TextField("Profile name", text: $name)
              }
            }

            VStack(alignment: .center, spacing: 0.0) {
              HStack(alignment: .center) {
                Spacer()

                Button(action: {
                  showing = false
                }) {
                  Label("Cancel", systemImage: "xmark")
                }

                Spacer()
                  .frame(width: 24.0)

                Button(action: {
                  settings.updateProfileName(profile!, name)
                  showing = false
                }) {
                  Label("Save", systemImage: "checkmark")
                    .buttonLabelStyle()
                }
                .prominentButtonStyle()

                Spacer()
              }
            }
          }
          .padding(6.0)
        }

        if !profile!.selected {
          Spacer().frame(height: 36.0)

          GroupBox(label: Text("Remove")) {
            VStack(alignment: .leading, spacing: 12.0) {
              HStack(alignment: .center) {
                Spacer()

                Button(action: {
                  settings.removeProfile(profile!)
                  showing = false
                }) {
                  Label("Remove profile", systemImage: "trash.fill")
                    .buttonLabelStyle()
                }
                .deleteButtonStyle()

                Spacer()
              }
            }
            .padding(6.0)
          }
        }
      }

      Button(action: {
        showing = false
      }) {
        Label("Close", systemImage: "xmark")
          .frame(minWidth: 0, maxWidth: .infinity)
      }
      .padding(.top, 24.0)
    }
    .padding()
    .frame(width: 400)
    .onAppear {
      name = profile?.name ?? ""
    }
  }
}

struct ProfileEditView_Previews: PreviewProvider {
  @State static var profile: Profile? = Profile(0, "", false)
  @State static var showing = true
  static var previews: some View {
    ProfileEditView(profile: $profile, showing: $showing)
  }
}
