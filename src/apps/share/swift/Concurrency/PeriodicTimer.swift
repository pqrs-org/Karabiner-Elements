// Keep this implementation instead of using swift-async-algorithms: with Xcode 27
// (Swift 6.4), its swift-collections 1.7.0 dependency introduces a strong reference
// to swift_initBorrow, which can cause launch-time crashes on older macOS versions
// whose Swift runtime lacks that symbol, on either Intel or Apple silicon.
// The crash was confirmed on an Intel Mac running macOS 15.7.5.
//
// Uses scheduled deadlines so time spent processing a tick does not shift later ticks.
struct PeriodicTimer: AsyncSequence, Sendable {
  typealias Element = ContinuousClock.Instant

  let interval: Duration

  struct AsyncIterator: AsyncIteratorProtocol {
    let interval: Duration
    private let clock = ContinuousClock()
    private var deadline: ContinuousClock.Instant?
    private var finished = false

    init(interval: Duration) {
      self.interval = interval
    }

    mutating func next() async -> Element? {
      guard !finished, !Task.isCancelled else { return nil }
      let nextDeadline = (deadline ?? clock.now).advanced(by: interval)
      do {
        try await clock.sleep(until: nextDeadline)
        try Task.checkCancellation()
      } catch {
        finished = true
        return nil
      }
      deadline = nextDeadline
      return clock.now
    }
  }

  func makeAsyncIterator() -> AsyncIterator {
    AsyncIterator(interval: interval)
  }
}
