import AppKit

private func complexModificationsFileImportJSONOutputCallback(
  _ json: UnsafePointer<CChar>,
  _ length: Int
) {
  let data = Data(bytes: json, count: length)

  MainActor.assumeIsolated {
    ComplexModificationsFileImport.shared.setParseResult(data)
  }
}

@MainActor
final class ComplexModificationsFileImport: ObservableObject {
  static let shared = ComplexModificationsFileImport()

  private struct ParseResult: Decodable {
    var title: String?
    var descriptions: [String]?
    var error: String?
  }

  private enum FileType {
    case json
    case javascript

    var fileExtension: String {
      switch self {
      case .json:
        return "json"
      case .javascript:
        return "js"
      }
    }
  }

  var task: URLSessionTask?
  private var parseResult: ParseResult?
  private var fileType = FileType.json

  @Published var fetching: Bool = false
  @Published var url: URL?
  @Published var error: String?
  @Published var fileData: Data?
  @Published var title: String = ""
  @Published var descriptions: [String] = []

  public func fetch(_ url: URL) {
    task?.cancel()

    self.url = url
    error = nil
    fileData = nil
    title = ""
    descriptions = []

    task = URLSession.shared.dataTask(with: url) { data, response, error in
      Task { @MainActor in
        self.fetching = false

        if let error = error {
          self.error = error.localizedDescription
          return
        }

        guard var nullTerminatedData = data else { return }

        // Treat the downloaded content as a null-terminated UTF-8 string to match the C API.
        // Data received from URLSession does not include a terminator, so append one before decoding.
        // String(validatingCString:) ignores any data after the first null character.
        nullTerminatedData.append(0)

        guard
          let code = nullTerminatedData.withUnsafeBytes({ buffer -> String? in
            guard let baseAddress = buffer.bindMemory(to: CChar.self).baseAddress else { return nil }
            return String(validatingCString: baseAddress)
          })
        else {
          self.error = "The downloaded file is not valid UTF-8."
          return
        }

        let fileData = Data(code.utf8)

        let pathExtension =
          response?.url?.pathExtension.lowercased() ?? url.pathExtension.lowercased()
        let candidates: [FileType]
        switch pathExtension {
        case "js":
          candidates = [.javascript]
        case "json":
          candidates = [.json]
        default:
          candidates = [.json, .javascript]
        }

        for candidate in candidates {
          if let result = self.parse(code, as: candidate), result.error == nil {
            self.fileType = candidate
            self.fileData = fileData
            self.title = result.title ?? ""
            self.descriptions = result.descriptions ?? []
            return
          }
        }

        self.error = self.parseResult?.error ?? "The downloaded file is not supported."
      }
    }

    fetching = true
    task?.resume()
  }

  fileprivate func setParseResult(_ data: Data) {
    parseResult = try? JSONDecoder().decode(ParseResult.self, from: data)
  }

  private func parse(_ code: String, as fileType: FileType) -> ParseResult? {
    parseResult = nil

    code.withCString { codeCString in
      fileType.fileExtension.withCString { fileExtensionCString in
        krbn_complex_modifications_assets_file_parse(
          codeCString,
          fileExtensionCString,
          complexModificationsFileImportJSONOutputCallback)
      }
    }

    return parseResult
  }

  public func save() {
    if let data = fileData {
      var buffer = [Int8](repeating: 0, count: 32 * 1024)
      krbn_get_user_complex_modifications_assets_directory(&buffer, buffer.count)
      guard let directory = String(utf8String: buffer) else { return }

      let time = Int(NSDate().timeIntervalSince1970)
      let path = URL(fileURLWithPath: "\(directory)/\(time).\(fileType.fileExtension)")

      do {
        try data.write(to: path)
      } catch {
        self.error = error.localizedDescription
      }
    }
  }
}
