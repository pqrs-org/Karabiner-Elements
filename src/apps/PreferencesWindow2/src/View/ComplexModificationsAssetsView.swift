import SwiftUI

struct ComplexModificationsAssetsView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @ObservedObject private var assetFiles = ComplexModificationsAssetFiles.shared

  var body: some View {
    VStack(alignment: .center, spacing: 12.0) {
      List {
        ForEach($assetFiles.files) { $assetFile in
          VStack(alignment: .leading, spacing: 8.0) {
            HStack(alignment: .center, spacing: 16.0) {
              Text(assetFile.title)
                .font(.title)

              Button(action: {
                Settings.shared.addComplexModificationRules(assetFile)
                contentViewStates.complexModificationsSheetPresented = false
              }) {
                Label("Enable All", systemImage: "plus.circle.fill")
                  .font(.caption)
              }

              Spacer()

              if assetFile.userFile {
                Button(action: {
                  assetFiles.removeFile(assetFile)
                }) {
                  Label("Remove", systemImage: "minus.circle.fill")
                    .font(.caption)
                }
              }
            }

            VStack(alignment: .leading, spacing: 8) {
              ForEach($assetFile.assetRules) { $assetRule in
                HStack(alignment: .center, spacing: 16.0) {
                  Text(assetRule.description)

                  Button(action: {
                    Settings.shared.addComplexModificationRule(assetRule)
                    contentViewStates.complexModificationsSheetPresented = false
                  }) {
                    Label("Enable", systemImage: "plus.circle.fill")
                  }
                }
              }
            }
            .padding(.leading, 32.0)
            .padding(.vertical, 16.0)

            Divider()
          }
        }
      }
      .background(Color(NSColor.textBackgroundColor))

      Spacer()

      Button(action: {
        NSWorkspace.shared.open(URL(string: "https://ke-complex-modifications.pqrs.org/")!)
      }) {
        Label(
          "Import more rules from the Internet (Open a web browser)",
          systemImage: "icloud.and.arrow.down.fill")
      }

      Button(action: {
        contentViewStates.complexModificationsSheetPresented = false
      }) {
        Label("Close", systemImage: "xmark")
          .frame(minWidth: 0, maxWidth: .infinity)
      }
    }
    .padding()
    .frame(width: 1000, height: 600)
    .onAppear {
      assetFiles.updateFiles()
    }
  }
}

struct ComplexModificationsAssetsView_Previews: PreviewProvider {
  static var previews: some View {
    ComplexModificationsAssetsView()
  }
}
