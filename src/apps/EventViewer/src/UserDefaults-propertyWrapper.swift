import Foundation
import SwiftUI

@propertyWrapper
struct UserDefault<T> {
    let key: String
    let defaultValue: T

    init(_ key: String, defaultValue: T) {
        self.key = key
        self.defaultValue = defaultValue
    }

    var wrappedValue: T {
        get {
            return UserDefaults.standard.object(forKey: key) as? T ?? defaultValue
        }
        nonmutating set {
            UserDefaults.standard.set(newValue, forKey: key)
        }
    }

    var projectedValue: Binding<T> {
        Binding<T>(
            get: {
                self.wrappedValue
            },
            set: {
                newValue in self.wrappedValue = newValue
            }
        )
    }
}
