import Foundation

@main
struct SettingsJSONDiffTests {
  static func main() {
    // The same recursive diff is used by settings updates and Changed Settings.
    let oldSettings: [String: Any] = [
      "nested": [
        "same": true,
        "value": 1,
      ],
      "array": [1, 2],
    ]
    let newSettings: [String: Any] = [
      "nested": [
        "same": true,
        "value": 2,
      ],
      "array": [2],
      "added": "ja",
    ]
    precondition(SettingsJSONDiff.make(from: oldSettings, to: oldSettings) == nil)
    let patch = SettingsJSONDiff.make(from: oldSettings, to: newSettings) as! [String: Any]
    precondition(
      NSDictionary(dictionary: patch).isEqual(to: [
        "nested": ["value": 2], "array": [2], "added": "ja",
      ]))
    precondition(SettingsJSONDiff.make(from: ["removed": true], to: [String: Any]()) == nil)

    print("Settings JSON diff tests passed")
  }
}
