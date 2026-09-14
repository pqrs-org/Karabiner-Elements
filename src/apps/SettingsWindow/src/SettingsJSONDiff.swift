import Foundation

// Settings snapshots have fixed object fields and no null values. Removed dictionary
// keys are intentionally omitted; dedicated APIs handle removal of devices and rules.
enum SettingsJSONDiff {
  // Returns only values added or changed in `newValue`; removed keys are omitted.
  static func make(from oldValue: Any, to newValue: Any) -> Any? {
    if let oldObject = oldValue as? [String: Any],
      let newObject = newValue as? [String: Any]
    {
      var difference: [String: Any] = [:]

      for (key, newChild) in newObject {
        if let oldChild = oldObject[key] {
          if let childDifference = make(from: oldChild, to: newChild) {
            difference[key] = childDifference
          }
        } else {
          difference[key] = newChild
        }
      }

      return difference.isEmpty ? nil : difference
    }

    if let oldObject = oldValue as? NSObject,
      oldObject.isEqual(newValue)
    {
      return nil
    }

    return newValue
  }
}
