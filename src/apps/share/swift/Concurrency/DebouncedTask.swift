// Keep this implementation instead of using swift-async-algorithms: with Xcode 27
// (Swift 6.4), its swift-collections 1.7.0 dependency introduces a strong reference
// to swift_initBorrow, which can cause launch-time crashes on older macOS versions
// whose Swift runtime lacks that symbol, on either Intel or Apple silicon.
// The crash was confirmed on an Intel Mac running macOS 15.7.5.
//
// Runs the latest action after input has been quiet for the given delay.
@MainActor
final class DebouncedTask {
  private var task: Task<Void, Never>?

  func schedule(after delay: Duration, action: @escaping @MainActor () -> Void) {
    cancel()
    task = Task {
      do {
        try await Task.sleep(for: delay)
        try Task.checkCancellation()
      } catch {
        return
      }
      action()
    }
  }

  func cancel() {
    task?.cancel()
    task = nil
  }

  deinit {
    task?.cancel()
  }
}
