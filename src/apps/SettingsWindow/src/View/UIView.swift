import SwiftUI

struct UIView: View {
  @ObservedObject private var appIcons = AppIcons.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 24.0) {
      GroupBox(label: Text("App icon")) {
        VStack(alignment: .leading, spacing: 12.0) {
          VStack {
            Label(
              "The app icon changes may take several seconds or require reopening the application.",
              systemImage: "lightbulb"
            )
            .padding()
            .foregroundColor(Color.warningForeground)
            .background(Color.warningBackground)
          }

          Picker(selection: $appIcons.selectedAppIconNumber, label: Text("")) {
            ForEach($appIcons.icons) { $appIcon in
              HStack {
                if let image = appIcon.karabinerElementsThumbnailImage {
                  Image(nsImage: image)
                    .resizable()
                    .frame(width: 64.0, height: 64.0)
                }

                if let image = appIcon.eventViewerThumbnailImage {
                  Image(nsImage: image)
                    .resizable()
                    .frame(width: 64.0, height: 64.0)
                }

                if let image = appIcon.multitouchExtensionThumbnailImage {
                  Image(nsImage: image)
                    .resizable()
                    .frame(width: 64.0, height: 64.0)
                }

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
