import SwiftUI

struct ComplexModificationsAssetsView: View {
  @Binding var showing: Bool
  @ObservedObject private var assetFiles = ComplexModificationsAssetFiles.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      Text("files \(assetFiles.files.count)")
      List {
        ForEach($assetFiles.files) { $assetFile in
          VStack(alignment: .leading, spacing: 8.0) {
            HStack(alignment: .center, spacing: 0) {
              Text(assetFile.title)

              Spacer()
            }

            ForEach($assetFile.assetRules) { $assetRule in
              HStack(alignment: .center, spacing: 0) {
                Text(assetRule.description)
                  .padding(.leading, 12.0)

                Button(action: {
                  showing = false
                }) {
                  Label("Enable", systemImage: "plus.circle.fill")
                }
              }
            }

            Divider()
          }
        }
      }
      .background(Color(NSColor.textBackgroundColor))

      Spacer()

      Button(action: {
        showing = false
      }) {
        Label("Close", systemImage: "xmark")
          .frame(minWidth: 0, maxWidth: .infinity)
      }
      .padding(.top, 24.0)
    }
    .padding()
    .frame(width: 1000, height: 600)
    .onAppear {
      assetFiles.updateFiles()
    }
  }
}

struct ComplexModificationsAssetsView_Previews: PreviewProvider {
  @State static var showing = true

  static var previews: some View {
    ComplexModificationsAssetsView(showing: $showing)
  }
}
