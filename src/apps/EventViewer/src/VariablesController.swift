import Cocoa

private func callback(_ filePath: UnsafePointer<Int8>?, _ context: UnsafeMutableRawPointer?) {
  if filePath == nil { return }

  let path = String(cString: filePath!)
  let obj: VariablesController? = unsafeBitCast(context, to: VariablesController.self)

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
public class VariablesController: NSObject {
  @IBOutlet var textView: NSTextView?

  deinit {
    libkrbn_disable_manipulator_environment_json_file_monitor()
  }

  @objc
  public func setup() {
    let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
    libkrbn_enable_manipulator_environment_json_file_monitor(callback, obj)
  }
}
