import AppKit

private func complexModificationsAssetsJSONOutputCallback(
  _ json: UnsafePointer<CChar>,
  _ length: Int
) {
  // The C++ JSON buffer is valid only during this callback, so copy it first.
  let data = Data(bytes: json, count: length)

  // `krbn_complex_modifications_assets_manager_reload` invokes this callback
  // synchronously from the main actor.
  MainActor.assumeIsolated {
    ComplexModificationsAssetFiles.shared.updateFiles(data)
  }
}

@MainActor
final class ComplexModificationsAssetFiles: ObservableObject {
  static let shared = ComplexModificationsAssetFiles()

  init() {
    krbn_enable_complex_modifications_assets_manager()
  }

  @Published var files: [ComplexModificationsAssetFile] = []

  public func updateFiles() {
    krbn_complex_modifications_assets_manager_reload(
      complexModificationsAssetsJSONOutputCallback)
  }

  fileprivate func updateFiles(_ data: Data) {
    let decoder = JSONDecoder()
    decoder.keyDecodingStrategy = .convertFromSnakeCase
    decoder.dateDecodingStrategy = .secondsSince1970

    do {
      files = try decoder.decode([ComplexModificationsAssetFile].self, from: data)
    } catch {
      print("Failed to decode complex modifications assets JSON: \(error)")
      files = []
    }
  }

  public func removeFile(_ complexModificationsAssetFile: ComplexModificationsAssetFile) {
    krbn_complex_modifications_assets_manager_erase_file(complexModificationsAssetFile.index)

    updateFiles()
  }
}
