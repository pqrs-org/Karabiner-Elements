import Cocoa

private func callback(_ bundleIdentifier: UnsafePointer<Int8>?,
                      _ filePath: UnsafePointer<Int8>?,
                      _ context: UnsafeMutableRawPointer?)
{
  if bundleIdentifier == nil { return }
  if filePath == nil { return }

  let identifier = String(cString: bundleIdentifier!)
  let path = String(cString: filePath!)
  let obj: FrontmostApplicationController? = unsafeBitCast(context, to: FrontmostApplicationController.self)

  if identifier == "org.pqrs.Karabiner-EventViewer" { return }

  // Update obj.text

  guard let font = NSFont(name: "Menlo", size: 11) else { return }
  let attributes = [
    NSAttributedString.Key.font: font,
    NSAttributedString.Key.foregroundColor: NSColor.textColor,
  ]

  if obj!.text.length > 4 * 1024 {
    obj!.text.setAttributedString(NSAttributedString(
      string: "", attributes: attributes
    ))
  }

  obj!.text.append(NSAttributedString(
    string: "Bundle Identifier:  \(identifier)\n",
    attributes: attributes
  ))
  obj!.text.append(NSAttributedString(
    string: "File Path:          \(path)\n\n",
    attributes: attributes
  ))

  // Update obj.textView

  DispatchQueue.main.async { [weak obj] in
    guard let obj = obj else { return }

    guard let textStorage = obj.textView!.textStorage else { return }

    textStorage.beginEditing()
    textStorage.setAttributedString(obj.text)
    textStorage.endEditing()

    obj.textView!.scrollRangeToVisible(NSMakeRange(obj.text.length, 0))
  }
}

@objc
public class FrontmostApplicationController: NSObject {
  @IBOutlet var textView: NSTextView?
  let text: NSMutableAttributedString
  let attributes: [NSAttributedString.Key: Any]

  override init() {
    text = NSMutableAttributedString()

    var attributes: [NSAttributedString.Key: Any] = [
      NSAttributedString.Key.foregroundColor: NSColor.textColor,
    ]
    if let font = NSFont(name: "Menlo", size: 11) {
      attributes[NSAttributedString.Key.font] = font
    }
    self.attributes = attributes

    super.init()
  }

  deinit {
    libkrbn_disable_frontmost_application_monitor()
  }

  @objc
  public func setup() {
    let obj = unsafeBitCast(self, to: UnsafeMutableRawPointer.self)
    libkrbn_enable_frontmost_application_monitor(callback, obj)

    let textStorage = textView?.textStorage
    textStorage?.beginEditing()
    textStorage?.setAttributedString(NSAttributedString(
      string: "Please switch to apps which you want to know Bundle Identifier.",
      attributes: attributes
    ))
    textStorage?.endEditing()
  }
}
