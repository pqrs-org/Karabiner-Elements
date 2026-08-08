import Foundation

enum CanonicalJSON {
  // Produces a stable representation for comparing JSON strings stored in karabiner.json
  // with entries loaded from simple_modifications.json, regardless of JSON object key order.
  static func string(fromJSONString string: String) -> String? {
    guard let data = string.data(using: .utf8),
      let object = try? JSONSerialization.jsonObject(with: data)
    else {
      return nil
    }

    return self.string(fromJSONObject: object)
  }

  // Produces the same stable representation while loading entries from
  // simple_modifications.json.
  static func string(fromJSONObject object: Any) -> String? {
    guard
      let data = try? JSONSerialization.data(
        withJSONObject: object,
        options: [.fragmentsAllowed, .sortedKeys, .withoutEscapingSlashes])
    else {
      return nil
    }

    return String(data: data, encoding: .utf8)
  }
}
