import AppKit
import SwiftUI

struct ChangedSettingsView: View {
  @ObservedObject private var localization = AppLocalization.shared
  @ObservedObject private var settings = Settings.shared
  @Environment(\.locale) private var locale

  var body: some View {
    let result = Result {
      let json = settings.configuration.changedSettingsJson
      let systemVersion = ProcessInfo.processInfo.operatingSystemVersion
      return (
        report: try ChangedSettings(json: json, locale: locale),
        clipboardText: try ChangedSettings.clipboardText(
          json: json,
          version: Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString")
            as? String,
          systemVersion:
            "\(systemVersion.majorVersion).\(systemVersion.minorVersion).\(systemVersion.patchVersion)"
        )
      )
    }
    VStack(alignment: .leading, spacing: 12) {
      AppLocalizedText("settings.changed_settings.title")
        .font(.title2)

      switch result {
      case .success(let (report, clipboardText)):
        Button {
          let pasteboard = NSPasteboard.general
          pasteboard.clearContents()
          pasteboard.writeObjects([clipboardText as NSString])
        } label: {
          AppLocalizedConstrainedLabel(
            "shared.action.copy_to_pasteboard", systemImage: "arrow.right.doc.on.clipboard")
        }

        if report.sections.isEmpty {
          AppLocalizedText("settings.changed_settings.empty")
            .foregroundColor(.secondary)
        }
        ScrollView {
          LazyVStack(alignment: .leading, spacing: 16) {
            ForEach(report.sections) { section in
              GroupBox {
                VStack(alignment: .leading, spacing: 10) {
                  ForEach(section.rows) { row in
                    HStack(alignment: .top, spacing: 16) {
                      Text(verbatim: row.label)
                        .frame(maxWidth: .infinity, alignment: .leading)
                      Text(verbatim: row.value)
                        .textSelection(.enabled)
                        .frame(maxWidth: .infinity, alignment: .trailing)
                    }
                    .help(row.id)
                  }
                }
                .padding(8)
              } label: {
                Text(verbatim: section.title)
              }
            }
          }
        }
      case .failure:
        AppLocalizedText("settings.changed_settings.error")
          .foregroundColor(.red)
      }
    }
    .padding()
  }
}
