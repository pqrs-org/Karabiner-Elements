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
          HStack(alignment: .center) {
            Text(description)
              .padding(.leading, 32)
              .font(.system(size: 24))
              .frame(maxWidth: .infinity, alignment: .leading)

            if !disabled {
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
              .padding(.leading, 24.0)
              .keyboardShortcut("s")
            }
          }
          .frame(maxWidth: .infinity, alignment: .leading)

          if disabled {
            Label(
              "Content is too large to edit. Please edit karabiner.json directly with your favorite editor.",
              systemImage: ErrorBorder.icon
            )
            .modifier(ErrorBorder())
          } else {
            if let errorMessage = errorMessage {
              Label(
                title: {
                  Text(errorMessage)
                    .textSelection(.enabled)
                },
                icon: {
                  Image(systemName: ErrorBorder.icon)
                }
              )
              .modifier(ErrorBorder())
            }

            CodeEditor(
              source: $jsonString, language: .json,
              theme: CodeEditor.ThemeName(
                rawValue: colorScheme == .dark ? "qtcreator_dark" : "qtcreator_light")
            )
            .border(Color(NSColor.separatorColor), width: 2)
          }
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
