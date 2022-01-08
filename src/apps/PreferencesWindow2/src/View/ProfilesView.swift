import SwiftUI

struct ProfilesView: View {
    @ObservedObject var settings = Settings.shared

    var body: some View {
        VStack(alignment: .leading, spacing: 12.0) {
            ScrollView {
                VStack(alignment: .leading, spacing: 0.0) {
                    // swiftformat:disable:next unusedArguments
                    ForEach($settings.profiles) { $profile in
                        HStack(alignment: .center, spacing: 0) {
                            Text(profile.name)

                            Spacer()

                            if profile.selected {
                                Label("Selected", systemImage: "checkmark.square")
                            } else {
                                Button(action: {
                                    settings.selectProfile(profile)
                                }) {
                                    Label("Select", systemImage: "square")
                                }
                            }

                            Button(action: {
                                settings.removeProfile(profile)
                            }) {
                                Label("Remove", systemImage: "minus.circle.fill")
                            }

                            .padding(.leading, 12.0)
                            .disabled(profile.selected)
                        }
                        .padding(12.0)
                        Divider()
                    }

                    Spacer()
                }
            }
            .background(Color(NSColor.textBackgroundColor))

            Button(action: {
                settings.appendProfile()
            }) {
                Label("Add new profile", systemImage: "plus.circle.fill")
            }
        }
        .padding()
    }
}

struct ProfilesView_Previews: PreviewProvider {
    static var previews: some View {
        ProfilesView()
    }
}
