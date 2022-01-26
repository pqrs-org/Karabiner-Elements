final class ComplexModificationsAssetFiles: ObservableObject {
  static let shared = ComplexModificationsAssetFiles()

  init() {
    libkrbn_enable_complex_modifications_assets_manager()
  }

  @Published var files: [ComplexModificationsAssetFile] = []

  public func updateFiles() {
    var newFiles: [ComplexModificationsAssetFile] = []

    libkrbn_complex_modifications_assets_manager_reload()

    let filesSize = libkrbn_complex_modifications_assets_manager_get_files_size()
    for fileIndex in 0..<filesSize {
      var rules: [ComplexModificationsAssetRule] = []

      let rulesSize = libkrbn_complex_modifications_assets_manager_get_rules_size(fileIndex)
      for ruleIndex in 0..<rulesSize {
        rules.append(
          ComplexModificationsAssetRule(
            fileIndex,
            ruleIndex,
            String(
              cString: libkrbn_complex_modifications_assets_manager_get_rule_description(
                fileIndex, ruleIndex
              )
            )
          ))
      }

      newFiles.append(
        ComplexModificationsAssetFile(
          fileIndex,
          String(
            cString: libkrbn_complex_modifications_assets_manager_get_file_title(fileIndex)),
          libkrbn_complex_modifications_assets_manager_user_file(fileIndex),
          rules
        ))
    }

    files = newFiles
  }
}
