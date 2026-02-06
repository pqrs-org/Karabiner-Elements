import AppKit
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
  @State private var hostingWindow: NSWindow?
  @State private var parentWindow: NSWindow?
  @State private var previousParentWindowLevel: NSWindow.Level?

  var body: some View {
    ZStack(alignment: .topLeading) {
      VStack(alignment: .leading, spacing: 12.0) {
        if rule != nil {
          VStack(alignment: .leading, spacing: 12.0) {
            Text(description)
              .padding(.leading, 32)
              .font(.system(size: 24))
              .frame(maxWidth: .infinity, alignment: .leading)

            if !disabled {
              HStack(alignment: .center) {
                if previousParentWindowLevel != nil {
                  Label(
                    "This window is pinned after opening in an external editor", systemImage: "pin")
                }

                Spacer()

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

                Button(
                  action: {
                    externalEditorController.chooseEditor()
                  },
                  label: {
                    Label("Choose editor", systemImage: "gear")
                      .buttonLabelStyle()
                  }
                )

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
                .padding(.leading, 36.0)
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
            if let errorMessage = errorMessage, evalErrorMessage == nil {
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
              source: $codeString,
              language: codeType == libkrbn_complex_modifications_rule_code_type_javascript
                ? .javascript
                : .json,
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
                Text("Result")
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
                Text("Log")
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
    .onChange(of: showing) { newValue in
      if !newValue {
        restoreWindowLevelIfNeeded()

        externalEditorController.reset()
        evalContinuation?.finish()
        evalContinuation = nil
        evalStreamTask?.cancel()
        evalStreamTask = nil
      }
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
    .background(
      WindowAccessor { window in
        if hostingWindow !== window {
          hostingWindow = window
          updateWindowLevelIfNeeded()
        }
      }
    )
    .onChange(of: didOpenExternalEditor) { _ in
      updateWindowLevelIfNeeded()
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

  private func updateWindowLevelIfNeeded() {
    guard didOpenExternalEditor, let window = hostingWindow else { return }

    // This view is shown in a sheet, so prefer adjusting the sheet parent window level.
    let target = window.sheetParent ?? window
    if parentWindow !== target {
      parentWindow = target
      previousParentWindowLevel = target.level
    } else if previousParentWindowLevel == nil {
      previousParentWindowLevel = target.level
    }

    target.level = .floating
  }

  private func restoreWindowLevelIfNeeded() {
    if let target = parentWindow, let previousParentWindowLevel = previousParentWindowLevel {
      target.level = previousParentWindowLevel
    }
    parentWindow = nil
    previousParentWindowLevel = nil
  }
}

private struct WindowAccessor: NSViewRepresentable {
  let onResolve: (NSWindow?) -> Void

  func makeNSView(context _: Context) -> NSView {
    let view = NSView()
    Task { @MainActor in
      onResolve(view.window)
    }
    return view
  }

  func updateNSView(_ nsView: NSView, context _: Context) {
    Task { @MainActor in
      onResolve(nsView.window)
    }
  }
}
