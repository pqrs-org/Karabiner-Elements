import AppKit
import AsyncAlgorithms
import CodeEditor
import SwiftUI

struct ComplexModificationsEditView: View {
  @Binding var rule: SettingsConfiguration.ComplexModificationsRule?
  @Binding var showing: Bool
  let onEditingCancelledByExternalChange: () -> Void
  @State private var description = ""
  @State private var disabled = true
  @State private var codeString = ""
  @State private var codeType = SettingsConfiguration.ComplexModificationsRule.CodeType.json
  @State private var errorMessage: String?
  @State private var expectedProfileIndex: Int?
  @State private var expectedRules: [SettingsConfiguration.ComplexModificationsRule]?
  @StateObject private var externalEditorController = ExternalEditorController.shared
  @State private var didOpenExternalEditor = false
  @ObservedObject private var settings = Settings.shared
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

            if !disabled {
              HStack(alignment: .center) {
                Spacer()

                Button(
                  action: {
                    externalEditorController.openEditor(
                      with: codeString,
                      fileExtension:
                        codeType == .javascript
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
              language: codeType == .javascript
                ? .javascript
                : .json,
              theme: CodeEditor.ThemeName(
                rawValue: colorScheme == .dark ? "qtcreator_dark" : "qtcreator_light")
            )
            .border(Color(NSColor.separatorColor), width: 2)

            if codeType == .javascript {
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

      codeType = rule?.codeType ?? .json
      expectedProfileIndex = selectedProfileIndex
      expectedRules = rules

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

            if disabled || codeType != .javascript {
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
    .onChange(of: monitoredConfiguration) { _ in
      cancelEditingIfTargetChangedExternally()
    }
  }

  private func save() -> Bool {
    guard editingTargetIsCurrent() else {
      cancelEditingIfTargetChangedExternally()
      return false
    }

    if rule!.index < 0 {
      errorMessage = settings.pushFrontComplexModificationsRule(
        codeString: codeString,
        codeType: codeType)
      if errorMessage == nil {
        updateEditedRuleAfterSave(index: 0)
        return true
      }
    } else {
      errorMessage = settings.replaceComplexModificationsRule(
        index: rule!.index,
        codeString: codeString,
        codeType: codeType)
      if errorMessage == nil {
        updateEditedRuleAfterSave(index: rule!.index)
        return true
      }
    }

    return false
  }

  private var selectedProfileIndex: Int? {
    settings.configuration.profiles.first { $0.selected }?.index
  }

  private var rules: [SettingsConfiguration.ComplexModificationsRule] {
    settings.configuration.selectedProfile.complexModifications.rules
  }

  private var monitoredConfiguration: MonitoredComplexModificationsConfiguration {
    MonitoredComplexModificationsConfiguration(
      selectedProfileIndex: selectedProfileIndex,
      rules: rules)
  }

  // A rule index is scoped to the selected profile and may become stale if the profile is changed
  // from the menu while this editor is open. The rule may also be moved, replaced, or removed by
  // directly editing karabiner.json. Dismiss the editor as soon as one of those changes is detected
  // so it cannot overwrite a different rule. Unrelated configuration changes remain safe.
  private func editingTargetIsCurrent() -> Bool {
    guard
      let expectedProfileIndex,
      expectedProfileIndex == selectedProfileIndex,
      let editedRule = rule
    else {
      return false
    }

    if editedRule.index < 0 {
      return expectedRules == rules
    }

    guard
      rules.indices.contains(editedRule.index),
      rules[editedRule.index] == editedRule
    else {
      return false
    }

    return true
  }

  private func cancelEditingIfTargetChangedExternally() {
    guard showing, !editingTargetIsCurrent() else {
      return
    }

    showing = false
    onEditingCancelledByExternalChange()
  }

  private func updateEditedRuleAfterSave(index: Int) {
    if rules.indices.contains(index) {
      rule = rules[index]
    } else {
      rule?.index = index
    }
    expectedRules = rules
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
      krbn_eval_js_to_json_string(
        code: codeCString,
        jsonBuffer: &jsonBuffer,
        jsonBufferLength: jsonBuffer.count,
        logMessageBuffer: &logBuffer,
        logMessageBufferLength: logBuffer.count,
        errorMessageBuffer: &errorBuffer,
        errorMessageBufferLength: errorBuffer.count
      )
    }

    let jsonString = String(utf8String: jsonBuffer) ?? ""
    let logMessages = String(utf8String: logBuffer) ?? ""
    let errorMessage = ok ? nil : String(utf8String: errorBuffer) ?? ""

    return (jsonString, logMessages, errorMessage?.isEmpty == true ? nil : errorMessage)
  }
}

private struct MonitoredComplexModificationsConfiguration: Equatable {
  let selectedProfileIndex: Int?
  let rules: [SettingsConfiguration.ComplexModificationsRule]
}
