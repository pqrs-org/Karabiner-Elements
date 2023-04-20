import SwiftUI

struct ComplexModificationsAssetsView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @ObservedObject private var assetFiles = ComplexModificationsAssetFiles.shared
  @State private var search = ""

  let formatter = DateFormatter()

  init() {
    formatter.locale = Locale(identifier: "en_US_POSIX")
    formatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
  }

  var body: some View {
    VStack(alignment: .center, spacing: 12.0) {
      HStack {
        Image(systemName: "magnifyingglass")
          .foregroundColor(.gray)

        TextField("Search", text: $search)
      }
      .padding(.vertical, 10.0)

      List {
        ForEach($assetFiles.files) { $assetFile in
          if search == "" || assetFile.match(search) {
            GroupBox(
              label:
                HStack(alignment: .bottom, spacing: 16.0) {
                  Text(assetFile.title)
                    .font(.title)

                  Spacer()
                }
            ) {
              VStack(alignment: .leading, spacing: 8.0) {
                ForEach($assetFile.assetRules) { $assetRule in
                  HStack(alignment: .center, spacing: 16.0) {
                    Text(assetRule.description)

                    Button(action: {
                      LibKrbn.Settings.shared.addComplexModificationRule(assetRule)
                      contentViewStates.complexModificationsViewSheetPresented = false
                    }) {
                      // Use `Image` and `Text` instead of `Label` to set icon color like `Button` in `List`.
                      Image(systemName: "plus.circle.fill").foregroundColor(.blue)
                      Text("Enable")
                    }

                    Spacer()
                  }

                  Divider()
                    .padding(.vertical, 4.0)
                }

                HStack {
                  Spacer()

                  if assetFile.userFile {
                    Text(
                      "Imported at \(formatter.string(from: assetFile.importedAt))"
                    )
                    .font(.caption)
                  }

                  Button(action: {
                    LibKrbn.Settings.shared.addComplexModificationRules(assetFile)
                    contentViewStates.complexModificationsViewSheetPresented = false
                  }) {
                    Text("Enable All")
                      .font(.caption)
                  }

                  if assetFile.userFile {
                    Button(action: {
                      assetFiles.removeFile(assetFile)
                    }) {
                      Image(systemName: "trash.fill")
                        .buttonLabelStyle()
                    }
                    .deleteButtonStyle()
                  }
                }
              }
              .padding()
            }
            .padding(.bottom, 32.0)
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
        contentViewStates.complexModificationsViewSheetPresented = false
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
