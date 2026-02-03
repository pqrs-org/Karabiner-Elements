import CodeEditor
import SwiftUI

struct ComplexModificationsEditView: View {
  @Binding var rule: LibKrbn.ComplexModificationsRule?
  @Binding var showing: Bool
  @State private var description = ""
  @State private var disabled = true
  @State private var codeString = ""
  @State private var errorMessage: String?
  @StateObject private var externalEditorController = ExternalEditorController.shared
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @Environment(\.colorScheme) var colorScheme

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .leading, spacing: 12.0) {
        if rule != nil {
          VStack(alignment: .leading, spacing: 12.0) {
            Text(description)
              .padding(.leading, 32)
              .font(.system(size: 24))
              .frame(maxWidth: .infinity, alignment: .leading)

            HStack(alignment: .center) {
              if !disabled {
                Button(
                  action: {
                    externalEditorController.openEditor(
                      with: codeString,
                      onError: { errorMessage = $0 },
                      onReload: {
                        codeString = $0
                        errorMessage = nil
                      }
                    )
                  },
                  label: {
                    Label(
                      externalEditorController.openTitle(), systemImage: "arrow.up.right.square"
                    )
                    .buttonLabelStyle()
                  }
                )
                .padding(.leading, 8.0)

                Button(
                  action: {
                    externalEditorController.chooseEditor()
                  },
                  label: {
                    Label("Choose editor", systemImage: "filemenu.and.selection")
                      .buttonLabelStyle()
                  }
                )
                .padding(.leading, 8.0)

                Spacer()

                Button(
                  action: {
                    if rule!.index < 0 {
                      errorMessage = settings.pushFrontComplexModificationsRule(codeString)
                      if errorMessage == nil {
                        showing = false
                      }
                    } else {
                      errorMessage = settings.replaceComplexModificationsRule(rule!, codeString)
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
              source: $codeString, language: .json,
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

      if let s = rule?.codeString {
        disabled = false
        codeString = s
      } else {
        disabled = true
        codeString = ""
      }

      externalEditorController.reset()
    }
    .onDisappear {
      externalEditorController.reset()
    }
    .onChange(of: codeString) { newValue in
      externalEditorController.syncFromAppEditor(
        text: newValue,
        onError: { errorMessage = $0 }
      )
    }
  }
}
