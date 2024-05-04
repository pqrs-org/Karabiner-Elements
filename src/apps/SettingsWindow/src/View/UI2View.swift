import SwiftUI

struct UI2View: View {
  @ObservedObject private var appIcons = AppIcons.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 24.0) {
      GroupBox(label: Text("App icon")) {
        VStack(alignment: .leading, spacing: 12.0) {
          VStack {
            Label(
              "It takes a few seconds for changes to the application icon to take effect.\nAnd to update the Dock icon, you need to close and reopen the application.",
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
              .overlay(
                RoundedRectangle(cornerRadius: 8)
                  .stroke(
                    Color(NSColor.selectedControlColor),
                    lineWidth: appIcons.selectedAppIconNumber == appIcon.id ? 3 : 0
                  )
              )

              .tag(appIcon.id)
            }
          }.pickerStyle(RadioGroupPickerStyle())

          Divider()
        }
        .padding(6.0)
        .background(Color(NSColor.textBackgroundColor))
      }

      Spacer()
    }
    .padding()
  }
}

struct UI2View_Previews: PreviewProvider {
  static var previews: some View {
    UI2View()
  }
}
