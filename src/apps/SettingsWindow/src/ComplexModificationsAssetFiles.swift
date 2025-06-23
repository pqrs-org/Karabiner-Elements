import AppKit

@MainActor
final class ComplexModificationsAssetFiles: ObservableObject {
  static let shared = ComplexModificationsAssetFiles()

  init() {
    libkrbn_enable_complex_modifications_assets_manager()
  }

  @Published var files: [LibKrbn.ComplexModificationsAssetFile] = []

  public func updateFiles() {
    var newFiles: [LibKrbn.ComplexModificationsAssetFile] = []

    libkrbn_complex_modifications_assets_manager_reload()

    let filesSize = libkrbn_complex_modifications_assets_manager_get_files_size()
    for fileIndex in 0..<filesSize {
      var buffer = [Int8](repeating: 0, count: 32 * 1024)

      var rules: [LibKrbn.ComplexModificationsAssetRule] = []

      let rulesSize = libkrbn_complex_modifications_assets_manager_get_rules_size(fileIndex)
      for ruleIndex in 0..<rulesSize {
        var ruleDescription = ""
        if libkrbn_complex_modifications_assets_manager_get_rule_description(
          fileIndex, ruleIndex, &buffer, buffer.count)
        {
          ruleDescription = String(utf8String: buffer) ?? ""
        }

        rules.append(LibKrbn.ComplexModificationsAssetRule(fileIndex, ruleIndex, ruleDescription))
      }

      var fileTitle = ""
      if libkrbn_complex_modifications_assets_manager_get_file_title(
        fileIndex, &buffer, buffer.count)
      {
        fileTitle = String(utf8String: buffer) ?? ""
      }

      newFiles.append(
        LibKrbn.ComplexModificationsAssetFile(
          fileIndex,
          fileTitle,
          libkrbn_complex_modifications_assets_manager_user_file(fileIndex),
          Date(
            timeIntervalSince1970: TimeInterval(
              libkrbn_complex_modifications_assets_manager_get_file_last_write_time(fileIndex))),
          rules
        ))
    }

    files = newFiles
  }

  public func removeFile(_ complexModificationsAssetFile: LibKrbn.ComplexModificationsAssetFile) {
    libkrbn_complex_modifications_assets_manager_erase_file(complexModificationsAssetFile.index)

    updateFiles()
  }
}
