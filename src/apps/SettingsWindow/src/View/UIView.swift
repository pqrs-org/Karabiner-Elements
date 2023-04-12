import SwiftUI

struct UIView: View {
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @ObservedObject private var appIcons = AppIcons.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 24.0) {
      GroupBox(label: Text("App icon")) {
        VStack(alignment: .leading, spacing: 12.0) {
          Picker(selection: $settings.appIcon, label: Text("")) {
            ForEach($appIcons.icons) { $appIcon in
              HStack {
                Image(decorative: appIcon.karabinerElementsThumbnailImageName)
                  .resizable()
                  .frame(width: 64.0, height: 64.0)

                Image(decorative: appIcon.eventViewerThumbnailImageName)
                  .resizable()
                  .frame(width: 64.0, height: 64.0)

                Image(decorative: appIcon.multitouchExtensionThumbnailImageName)
                  .resizable()
                  .frame(width: 64.0, height: 64.0)

                Spacer()
              }
              .padding(.vertical, 5.0)
              .tag(appIcon.id)
            }
          }.pickerStyle(RadioGroupPickerStyle())

          Divider()
        }
        .padding(6.0)
      }

      Spacer()
    }
    .padding()
  }
}

struct UIView_Previews: PreviewProvider {
  static var previews: some View {
    UIView()
  }
}
