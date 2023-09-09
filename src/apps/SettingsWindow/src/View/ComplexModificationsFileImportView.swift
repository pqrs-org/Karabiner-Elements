import SwiftUI

struct ComplexModificationsFileImportView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @ObservedObject private var complexModificationsFileImport = ComplexModificationsFileImport.shared

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      if complexModificationsFileImport.fetching {
        HStack(alignment: .center, spacing: 8.0) {
          Spacer()

          ProgressView()

          Text("Loading...")

          Spacer()
        }
      } else {
        Text("Import file from \(complexModificationsFileImport.url?.absoluteString ?? "")")

        if let error = complexModificationsFileImport.error {
          Label(error, systemImage: "exclamationmark.circle.fill")
        }

        List {
          Text(complexModificationsFileImport.title)
            .font(.title)

          VStack(alignment: .leading, spacing: 8) {
            ForEach(complexModificationsFileImport.descriptions, id: \.self) { description in
              Text(description)
            }
          }
          .padding(.leading, 32.0)
          .padding(.vertical, 16.0)
        }
        .background(Color(NSColor.textBackgroundColor))
      }

      HStack(alignment: .center) {
        Spacer()

        Button(
          action: {
            contentViewStates.complexModificationsViewSheetPresented = false
          },
          label: {
            Label("Cancel", systemImage: "xmark")
          })

        Spacer()
          .frame(width: 24.0)

        Button(
          action: {
            complexModificationsFileImport.save()
            ComplexModificationsAssetFiles.shared.updateFiles()

            contentViewStates.complexModificationsViewSheetView =
              ComplexModificationsSheetView.assets
          },
          label: {
            Label("Import", systemImage: "tray.and.arrow.down.fill")
              .buttonLabelStyle()
          }
        )
        .prominentButtonStyle()
        .disabled(complexModificationsFileImport.jsonData == nil)

        Spacer()
      }
    }
    .padding()
    .frame(width: 1000, height: 300)
  }
}

struct ComplexModificationsFileImportView_Previews: PreviewProvider {
  static var previews: some View {
    ComplexModificationsFileImportView()
  }
}
