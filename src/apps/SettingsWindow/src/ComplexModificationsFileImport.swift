import AppKit

@MainActor
final class ComplexModificationsFileImport: ObservableObject {
  static let shared = ComplexModificationsFileImport()

  var task: URLSessionTask?

  @Published var fetching: Bool = false
  @Published var url: URL?
  @Published var error: String?
  @Published var jsonData: Data?
  @Published var title: String = ""
  @Published var descriptions: [String] = []

  public func fetchJson(_ url: URL) {
    task?.cancel()

    self.url = url
    error = nil
    jsonData = nil
    title = ""
    descriptions = []

    task = URLSession.shared.dataTask(with: url) { data, _, error in
      guard let data = data else { return }

      Task { @MainActor in
        self.fetching = false

        if let error = error {
          self.error = error.localizedDescription
        } else {
          do {
            self.jsonData = data
            let json = try JSONSerialization.jsonObject(with: data) as? [String: Any]

            self.title = json?["title"] as? String ?? ""
            for rule in (json?["rules"] as? [[String: Any]] ?? []) {
              self.descriptions.append(rule["description"] as? String ?? "")
            }
          } catch {
            self.jsonData = nil
            self.error = error.localizedDescription
          }
        }
      }
    }

    fetching = true
    task?.resume()
  }

  public func save() {
    if let data = self.jsonData {
      var buffer = [Int8](repeating: 0, count: 32 * 1024)
      libkrbn_get_user_complex_modifications_assets_directory(&buffer, buffer.count)
      guard let directory = String(utf8String: buffer) else { return }

      let time = Int(NSDate().timeIntervalSince1970)
      let path = URL(fileURLWithPath: "\(directory)/\(time).json")

      do {
        try data.write(to: path)
      } catch {
        self.error = error.localizedDescription
      }
    }
  }
}
