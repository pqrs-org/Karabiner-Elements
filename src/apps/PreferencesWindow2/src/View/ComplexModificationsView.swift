import SwiftUI

enum ComplexModificationsSheetView: String {
  case assets
  case fileImport
}

struct ComplexModificationsView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @ObservedObject private var settings = Settings.shared
  @State private var moveDisabled: Bool = true

  var body: some View {
    VStack(alignment: .leading, spacing: 12.0) {
      List {
        ForEach($settings.complexModificationsRules) { $complexModificationRule in
          VStack {
            HStack(alignment: .center, spacing: 0) {
              if settings.complexModificationsRules.count > 1 {
                Image(systemName: "arrow.up.arrow.down.square.fill")
                  .resizable(resizingMode: .stretch)
                  .frame(width: 16.0, height: 16.0)
                  .onHover { hovering in
                    moveDisabled = !hovering
                  }
              }

              Text(complexModificationRule.description)
                .padding(.leading, 6.0)

              Spacer()

              Button(action: {
                settings.removeComplexModificationsRule(complexModificationRule)
              }) {
                Label("Remove", systemImage: "minus.circle.fill")
              }
            }

            Divider()
          }
          .moveDisabled(moveDisabled)
        }.onMove { indices, destination in
          if let first = indices.first {
            settings.moveComplexModificationsRule(first, destination)
          }
        }
      }
      .background(Color(NSColor.textBackgroundColor))

      Spacer()

      if settings.complexModificationsRules.count > 1 {
        HStack {
          Text("You can reorder list by dragging")
          Image(systemName: "arrow.up.arrow.down.square.fill")
            .resizable(resizingMode: .stretch)
            .frame(width: 16.0, height: 16.0)
          Text("icon")
        }
      }

      Button(action: {
        contentViewStates.complexModificationsSheetView = ComplexModificationsSheetView.assets
        contentViewStates.complexModificationsSheetPresented = true
      }) {
        Label("Add rule", systemImage: "plus.circle.fill")
      }
    }
    .padding()
    .sheet(isPresented: $contentViewStates.complexModificationsSheetPresented) {
      if let sheetView = contentViewStates.complexModificationsSheetView {
        switch sheetView {
        case ComplexModificationsSheetView.assets:
          ComplexModificationsAssetsView()
        case ComplexModificationsSheetView.fileImport:
          ComplexModificationsFileImportView()
        }
      }
    }
  }
}

struct ComplexModificationsView_Previews: PreviewProvider {
  static var previews: some View {
    ComplexModificationsView()
  }
}
