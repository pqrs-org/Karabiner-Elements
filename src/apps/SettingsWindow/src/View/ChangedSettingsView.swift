import AppKit
import SwiftUI

struct ChangedSettingsView: View {
  @ObservedObject private var localization = AppLocalization.shared
  @ObservedObject private var settings = Settings.shared
  @Environment(\.locale) private var locale

  private func text(_ key: String) -> String {
    AppLanguage.text(key, locale: locale)
  }

  var body: some View {
    let result = Result {
      try ChangedSettings(json: settings.configuration.changedSettingsJson, locale: locale)
    }
    VStack(alignment: .leading, spacing: 12) {
      AppLocalizedText("changed_settings.title")
        .font(.title2)

      switch result {
      case .success(let report):
        Button {
          let version =
            Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String
            ?? "Unknown"
          var content = report.text(
            version: version, systemVersion: ProcessInfo.processInfo.operatingSystemVersionString)
          if report.sections.isEmpty { content += "\n\n" + text("changed_settings.empty") }
          let pasteboard = NSPasteboard.general
          pasteboard.clearContents()
          pasteboard.writeObjects([content as NSString])
        } label: {
          AppLocalizedLabel(
            "shared.action.copy_to_pasteboard", systemImage: "arrow.right.doc.on.clipboard")
        }

        if report.sections.isEmpty {
          AppLocalizedText("changed_settings.empty")
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
        AppLocalizedText("changed_settings.error")
          .foregroundColor(.red)
      }
    }
    .padding()
  }
}
