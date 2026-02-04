import AsyncAlgorithms
import CodeEditor
import SwiftUI

struct ComplexModificationsEditView: View {
  @Binding var rule: LibKrbn.ComplexModificationsRule?
  @Binding var showing: Bool
  @State private var description = ""
  @State private var disabled = true
  @State private var codeString = ""
  @State private var codeType = libkrbn_complex_modifications_rule_code_type_json
  @State private var errorMessage: String?
  @StateObject private var externalEditorController = ExternalEditorController.shared
  @State private var didOpenExternalEditor = false
  @ObservedObject private var settings = LibKrbn.Settings.shared
  @Environment(\.colorScheme) var colorScheme

  @State private var evalResultString = ""
  @State private var evalLogMessages = ""
  @State private var evalErrorMessage: String?
  @State private var evalContinuation: AsyncStream<String>.Continuation?
  @State private var evalStreamTask: Task<Void, Never>?

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
                      fileExtension:
                        codeType == libkrbn_complex_modifications_rule_code_type_javascript
                        ? "js"
                        : "json",
                      onError: { errorMessage = $0 },
                      onReload: {
                        codeString = $0
                        _ = save()
                      }
                    )

                    didOpenExternalEditor = true
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
                    if save() {
                      showing = false
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

            if didOpenExternalEditor {
              Label(
                title: {
                  Text(
                    "Changes saved in the external editor are automatically reflected while this window is open."
                  )
                  .textSelection(.enabled)
                },
                icon: {
                  Image(systemName: InfoBorder.icon)
                }
              )
              .modifier(InfoBorder())
            }

            CodeEditor(
              source: $codeString, language: .json,
              theme: CodeEditor.ThemeName(
                rawValue: colorScheme == .dark ? "qtcreator_dark" : "qtcreator_light")
            )
            .border(Color(NSColor.separatorColor), width: 2)

            if codeType == libkrbn_complex_modifications_rule_code_type_javascript {
              if let evalErrorMessage = evalErrorMessage {
                Label(
                  title: {
                    Text(evalErrorMessage)
                      .textSelection(.enabled)
                  },
                  icon: {
                    Image(systemName: ErrorBorder.icon)
                  }
                )
                .modifier(ErrorBorder())
              }

              VStack(alignment: .leading, spacing: 6) {
                Text("JavaScript Result")
                  .font(.headline)

                ScrollView {
                  Text(evalResultString)
                    .font(.callout)
                    .monospaced()
                    .textSelection(.enabled)
                    .frame(maxWidth: .infinity, alignment: .topLeading)
                    .padding(8)
                }
                .frame(maxWidth: .infinity, minHeight: 120, maxHeight: 120)
                .background(Color(NSColor.textBackgroundColor))
                .border(Color(NSColor.separatorColor), width: 2)
              }

              VStack(alignment: .leading, spacing: 6) {
                Text("Console Log")
                  .font(.headline)

                ScrollView {
                  Text(evalLogMessages.isEmpty ? "(no log output)" : evalLogMessages)
                    .font(.callout)
                    .monospaced()
                    .textSelection(.enabled)
                    .frame(maxWidth: .infinity, alignment: .topLeading)
                    .padding(8)
                }
                .frame(maxWidth: .infinity, minHeight: 60, maxHeight: 60)
                .background(Color(NSColor.textBackgroundColor))
                .border(Color(NSColor.separatorColor), width: 2)
              }
            }
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

      codeType = rule?.codeType ?? libkrbn_complex_modifications_rule_code_type_json

      externalEditorController.reset()

      if evalContinuation == nil {
        let stream = AsyncStream<String> { continuation in
          evalContinuation = continuation
        }

        evalStreamTask = Task {
          for await code in stream.debounce(for: .milliseconds(500)) {
            if Task.isCancelled {
              break
            }

            if disabled || codeType != libkrbn_complex_modifications_rule_code_type_javascript {
              await MainActor.run {
                evalResultString = ""
                evalLogMessages = ""
                evalErrorMessage = nil
              }
              continue
            }

            let result = evaluateJavascript(code: code)
            await MainActor.run {
              if codeString != code {
                return
              }

              evalResultString = result.jsonString
              evalLogMessages = result.logMessages
              evalErrorMessage = result.errorMessage
            }
          }
        }
      }

      evalContinuation?.yield(codeString)
    }
    .onDisappear {
      externalEditorController.reset()
      evalContinuation?.finish()
      evalContinuation = nil
      evalStreamTask?.cancel()
      evalStreamTask = nil
    }
    .onChange(of: codeString) { newValue in
      externalEditorController.syncFromAppEditor(
        text: newValue,
        onError: { errorMessage = $0 }
      )

      evalContinuation?.yield(newValue)
    }
    .onChange(of: codeType) { _ in
      evalContinuation?.yield(codeString)
    }
  }

  private func save() -> Bool {
    if rule!.index < 0 {
      errorMessage = settings.pushFrontComplexModificationsRule(
        codeString, codeType)
      if errorMessage == nil {
        // Set index to call replaceComplexModificationsRule on the next save.
        rule!.index = 0
        return true
      }
    } else {
      errorMessage = settings.replaceComplexModificationsRule(
        rule!, codeString, codeType)
      if errorMessage == nil {
        return true
      }
    }

    return false
  }

  private func evaluateJavascript(code: String) -> (
    jsonString: String, logMessages: String, errorMessage: String?
  ) {
    let bufferSize = 256 * 1024
    let logBufferSize = 256 * 1024
    var jsonBuffer = [Int8](repeating: 0, count: bufferSize)
    var logBuffer = [Int8](repeating: 0, count: logBufferSize)
    var errorBuffer = [Int8](repeating: 0, count: 4 * 1024)

    let ok = code.withCString { codeCString in
      libkrbn_eval_js_to_json_string(
        codeCString,
        &jsonBuffer,
        jsonBuffer.count,
        &logBuffer,
        logBuffer.count,
        &errorBuffer,
        errorBuffer.count
      )
    }

    let jsonString = String(utf8String: jsonBuffer) ?? ""
    let logMessages = String(utf8String: logBuffer) ?? ""
    let errorMessage = ok ? nil : String(utf8String: errorBuffer) ?? ""

    return (jsonString, logMessages, errorMessage?.isEmpty == true ? nil : errorMessage)
  }
}
