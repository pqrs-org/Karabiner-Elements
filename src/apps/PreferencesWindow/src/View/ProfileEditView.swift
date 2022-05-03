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
    }
    .padding()
    .frame(width: 400)
    .onAppear {
      name = profile?.name ?? ""
    }
  }
}

struct ProfileEditView_Previews: PreviewProvider {
    @State static var profile: LibKrbn.Profile? = LibKrbn.Profile(0, "", false)
  @State static var showing = true
  static var previews: some View {
    ProfileEditView(profile: $profile, showing: $showing)
  }
}
