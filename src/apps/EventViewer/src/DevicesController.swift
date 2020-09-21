import Cocoa

private func callback(_ filePath: UnsafePointer<Int8>?, _ context: UnsafeMutableRawPointer?) {
  if filePath == nil { return }

  let path = String(cString: filePath!)
  let obj: DevicesController? = unsafeBitCast(context, to: DevicesController.self)

  guard let message = try? String(contentsOfFile: path, encoding: .utf8) else { return }

  DispatchQueue.main.async { [weak obj] in
    guard let obj = obj else { return }

    guard let textStorage = obj.textView!.textStorage else { return }
    guard let font = NSFont(name: "Menlo", size: 11) else { return }
    let attributedString = NSAttributedString(string: message, attributes: [
      NSAttributedString.Key.font: font,
      NSAttributedString.Key.foregroundColor: NSColor.textColor,
    ])

    textStorage.beginEditing()
    textStorage.setAttributedString(attributedString)
    textStorage.endEditing()
  }
}

@objc
public class DevicesController: NSObject {
  @IBOutlet var textView: NSTextView?

  deinit {
    libkrbn_disable_device_details_json_file_monitor()
  }

  @objc
  public func setup() {
    let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
    libkrbn_enable_device_details_json_file_monitor(callback, obj)
  }
}
