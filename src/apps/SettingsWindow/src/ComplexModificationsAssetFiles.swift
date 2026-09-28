import AppKit

@MainActor
final class ComplexModificationsAssetFiles: ObservableObject {
  static let shared = ComplexModificationsAssetFiles()

  private var lastRevision: UInt64 = 0

  @Published var files: [ComplexModificationsAssetFile] = []

  public func updateFiles() {
    sendConfigurationCommand(.reloadAssets) { data in
      Self.shared.applyResponse(data)
    }
  }

  private func applyResponse(_ data: Data) {
    guard let result = (try? JSONSerialization.jsonObject(with: data)) as? [String: Any],
      let revision = result["revision"] as? UInt64, revision > lastRevision,
      let files = result["files"], let data = try? JSONSerialization.data(withJSONObject: files)
    else { return }
    lastRevision = revision
    updateFiles(data)
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
    sendConfigurationCommand(
      .eraseAsset,
      parameters: [
        "file_path": complexModificationsAssetFile.filePath
      ]
    ) { data in
      Self.shared.applyResponse(data)
    }
  }
}
