final class ComplexModificationsFileImport: ObservableObject {
  static let shared = ComplexModificationsFileImport()

  var task: URLSessionTask?

  @Published var fetching: Bool = false
  @Published var url: URL?
  @Published var error: String?
  @Published var jsonData: [String: Any]?
  @Published var title: String = ""
  @Published var descriptions: [String] = []

  public func fetchJson(_ url: URL) {
    task?.cancel()

    self.url = url
    error = nil
    jsonData = nil
    title = ""
    descriptions = []

    task = URLSession.shared.dataTask(with: url) { [weak self] (data, response, error) in
      guard let self = self else { return }
      guard let data = data else { return }

      self.fetching = false

      do {
        self.jsonData = try JSONSerialization.jsonObject(with: data) as? [String: Any]

        self.title = self.jsonData?["title"] as! String
        for rule in (self.jsonData?["rules"] as! [[String: Any]]) {
          self.descriptions.append(rule["description"] as! String)
        }
      } catch {
        self.error = error.localizedDescription
      }
    }

    fetching = true
    task?.resume()
  }
}
