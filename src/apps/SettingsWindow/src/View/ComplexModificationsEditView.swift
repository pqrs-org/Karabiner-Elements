import CodeEditor
import SwiftUI

struct ComplexModificationsEditView: View {
  @Binding var rule: LibKrbn.ComplexModificationsRule?
  @Binding var showing: Bool
  @State private var description = ""
  @State private var disabled = true
  @State private var jsonString = ""
  @State private var errorMessage: String?
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @Environment(\.colorScheme) var colorScheme

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .leading, spacing: 12.0) {
        if rule != nil {
          VStack(alignment: .leading, spacing: 12.0) {
            HStack(alignment: .center) {
              Text(description)
                .padding(.leading, 32)
                .font(.system(size: 24))

              Spacer()

              if !disabled {
                Spacer()
                  .frame(width: 24.0)

                Button(
                  action: {
                    if rule!.index < 0 {
                      errorMessage = settings.pushFrontComplexModificationsRule(jsonString)
                      if errorMessage == nil {
                        showing = false
                      }
                    } else {
                      errorMessage = settings.replaceComplexModificationsRule(rule!, jsonString)
                      if errorMessage == nil {
                        showing = false
                      }
                    }
                  },
                  label: {
                    Label("Save", systemImage: "checkmark")
                      .buttonLabelStyle()
                  }
                )
                .buttonStyle(BorderedProminentButtonStyle())
                .keyboardShortcut("s")
              }
            }

            if disabled {
              Label(
                "Content is too large to edit. Please edit karabiner.json directly with your favorite editor.",
                systemImage: "exclamationmark.circle.fill"
              )
              .modifier(ErrorBorder())
            } else {
              if let errorMessage = errorMessage {
                Label(
                  errorMessage,
                  systemImage: "exclamationmark.circle.fill"
                )
                .modifier(ErrorBorder())
              }

              CodeEditor(
                source: $jsonString, language: .json,
                theme: CodeEditor.ThemeName(
                  rawValue: colorScheme == .dark ? "qtcreator_dark" : "qtcreator_light")
              )
              .border(Color(NSColor.textColor), width: 1)
            }

            Spacer()
          }
          .padding(12.0)
        }
      }

      SheetCloseButton {
        showing = false
      }
    }
    .padding()
    .frame(width: 1000, height: 600)
    .onAppear {
      description = rule?.description ?? ""

      if let s = rule?.jsonString {
        disabled = false
        jsonString = s
      } else {
        disabled = true
        jsonString = ""
      }
    }
  }
}
