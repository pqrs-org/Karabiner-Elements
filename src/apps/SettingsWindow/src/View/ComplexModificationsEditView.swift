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
    VStack(alignment: .leading, spacing: 12.0) {
      if rule != nil {
        VStack(alignment: .leading, spacing: 12.0) {
          HStack(alignment: .center) {
            Text(description)

            Spacer()

            Button(
              action: {
                showing = false
              },
              label: {
                Label(disabled ? "Close" : "Cancel", systemImage: "xmark")
              })

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
              .prominentButtonStyle()
            }
          }

          if disabled {
            VStack {
              Text(
                "Content is too large to edit. Please edit karabiner.json directly with your favorite editor."
              )
            }
            .padding()
            .foregroundColor(Color.errorForeground)
            .background(Color.errorBackground)
          } else {
            if errorMessage != nil {
              VStack {
                Text(errorMessage!)
              }
              .padding()
              .foregroundColor(Color.errorForeground)
              .background(Color.errorBackground)
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
        .padding(6.0)
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

struct ComplexModificationsEditView_Previews: PreviewProvider {
  @State static var rule: LibKrbn.ComplexModificationsRule? = LibKrbn.ComplexModificationsRule(
    0, "", "{}")
  @State static var showing = true
  static var previews: some View {
    ComplexModificationsEditView(rule: $rule, showing: $showing)
  }
}
