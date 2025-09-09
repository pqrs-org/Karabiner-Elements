import SwiftUI

struct ComplexModificationsAssetsView: View {
  @ObservedObject private var contentViewStates = ContentViewStates.shared
  @ObservedObject private var assetFiles = ComplexModificationsAssetFiles.shared
  @State private var search = ""
  @State private var hoverRuleId: UUID?

  let formatter = DateFormatter()

  init() {
    formatter.locale = Locale(identifier: "en_US_POSIX")
    formatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
  }

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .center, spacing: 12.0) {
        Button(
          action: {
            NSWorkspace.shared.open(URL(string: "https://ke-complex-modifications.pqrs.org/")!)
          },
          label: {
            AccentColorIconLabel(
              title: "Import more rules from the Internet (Open a web browser)",
              systemImage: "icloud.and.arrow.down.fill")
          }
        )
        .padding(.top, 10.0)

        HStack(alignment: .center) {
          Image(systemName: "magnifyingglass")
            .foregroundColor(.gray)

          TextField("Search", text: $search)
        }

        List {
          ForEach($assetFiles.files) { $assetFile in
            if search == "" || assetFile.match(search) {
              GroupBox(
                label:
                  Text(assetFile.title)
                  .font(.title)
              ) {
                VStack(alignment: .leading, spacing: 8.0) {
                  ForEach($assetFile.assetRules) { $assetRule in
                    HStack(alignment: .center, spacing: 16.0) {
                      Text(assetRule.description)
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .if(hoverRuleId == assetRule.id) {
                          $0.overlay(
                            RoundedRectangle(cornerRadius: 2)
                              .inset(by: -4)
                              .stroke(
                                Color(NSColor(Color.accentColor)),
                                lineWidth: 1
                              )
                          )
                        }

                      Button(
                        action: {
                          LibKrbn.Settings.shared.addComplexModificationRule(assetRule)
                          contentViewStates.complexModificationsViewSheetPresented = false
                        },
                        label: {
                          // Use `Image` and `Text` instead of `Label` to set icon color like `Button` in `List`.
                          Image(systemName: "plus.circle.fill").foregroundColor(.blue)
                          Text("Enable")
                        }
                      )
                      .onHover { hovering in
                        if hovering {
                          hoverRuleId = assetRule.id
                        } else {
                          if hoverRuleId == assetRule.id {
                            hoverRuleId = nil
                          }
                        }
                      }
                    }

                    Divider()
                  }

                  HStack {
                    if assetFile.userFile {
                      Text(
                        "Imported at \(formatter.string(from: assetFile.importedAt))"
                      )
                      .font(.caption)
                    }

                    Button(
                      action: {
                        LibKrbn.Settings.shared.addComplexModificationRules(assetFile)
                        contentViewStates.complexModificationsViewSheetPresented = false
                      },
                      label: {
                        Text("Enable All")
                          .font(.caption)
                      })

                    if assetFile.userFile {
                      Button(
                        role: .destructive,
                        action: {
                          assetFiles.removeFile(assetFile)
                        },
                        label: {
                          Image(systemName: "trash")
                            .buttonLabelStyle()
                        }
                      )
                      .deleteButtonStyle()
                    }
                  }
                  .frame(maxWidth: .infinity, alignment: .trailing)
                }
                .padding()
              }
              .padding(.bottom, 32.0)
            }
          }
        }
        .background(Color(NSColor.textBackgroundColor))
      }

      SheetCloseButton {
        contentViewStates.complexModificationsViewSheetPresented = false
      }
    }
    .padding()
    .frame(width: 1000, height: 600)
    .onAppear {
      assetFiles.updateFiles()
    }
  }
}
