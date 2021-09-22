import SwiftUI

private func callback(_ filePath: UnsafePointer<Int8>?, _ context: UnsafeMutableRawPointer?) {
    if filePath == nil { return }

    let path = String(cString: filePath!)
    let obj: Devices! = unsafeBitCast(context, to: Devices.self)

    guard let text = try? String(contentsOfFile: path, encoding: .utf8) else { return }

    DispatchQueue.main.async { [weak obj] in
        guard let obj = obj else { return }
        
        obj.text = text
    }
}

public class Devices: ObservableObject {
    public static let shared = Devices()

    @Published var text = ""

    init() {
        let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
        libkrbn_enable_device_details_json_file_monitor(callback, obj)
    }

    deinit {
        libkrbn_disable_device_details_json_file_monitor()
    }
}
