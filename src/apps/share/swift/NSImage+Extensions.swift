import AppKit

extension NSImage {
  func removingTransparentPadding(alphaThreshold: UInt8 = 1) -> NSImage {
    let maximumPixelDimension =
      representations
      .map { max($0.pixelsWide, $0.pixelsHigh) }
      .max() ?? Int(max(size.width, size.height))
    let imageDimension = max(
      CGFloat(maximumPixelDimension),
      max(size.width, size.height)
    )
    var proposedRect = CGRect(
      origin: .zero,
      size: CGSize(width: imageDimension, height: imageDimension)
    )
    guard
      let sourceImage = cgImage(
        forProposedRect: &proposedRect,
        context: nil,
        hints: nil
      )
    else {
      return self
    }

    let width = sourceImage.width
    let height = sourceImage.height
    let bytesPerPixel = 4
    let bytesPerRow = width * bytesPerPixel

    guard
      let colorSpace = CGColorSpace(name: CGColorSpace.sRGB),
      let context = CGContext(
        data: nil,
        width: width,
        height: height,
        bitsPerComponent: 8,
        bytesPerRow: bytesPerRow,
        space: colorSpace,
        bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue
      ),
      let data = context.data
    else {
      return self
    }

    context.clear(CGRect(x: 0, y: 0, width: width, height: height))
    context.draw(sourceImage, in: CGRect(x: 0, y: 0, width: width, height: height))

    let pixels = data.assumingMemoryBound(to: UInt8.self)
    var minimumX = width
    var minimumY = height
    var maximumX = -1
    var maximumY = -1

    for y in 0..<height {
      for x in 0..<width {
        let alpha = pixels[y * bytesPerRow + x * bytesPerPixel + 3]
        if alpha > alphaThreshold {
          minimumX = min(minimumX, x)
          minimumY = min(minimumY, y)
          maximumX = max(maximumX, x)
          maximumY = max(maximumY, y)
        }
      }
    }

    guard maximumX >= minimumX, maximumY >= minimumY else {
      return self
    }

    let cropRect = CGRect(
      x: minimumX,
      y: minimumY,
      width: maximumX - minimumX + 1,
      height: maximumY - minimumY + 1
    )
    guard let croppedImage = context.makeImage()?.cropping(to: cropRect) else {
      return self
    }

    return NSImage(
      cgImage: croppedImage,
      size: NSSize(width: cropRect.width, height: cropRect.height)
    )
  }
}
