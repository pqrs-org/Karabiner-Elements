@main
struct ConcurrencyTests {
  @MainActor
  static func main() async throws {
    let debounce = DebouncedTask()
    var values: [Int] = []
    debounce.schedule(after: .seconds(60)) { values.append(1) }
    debounce.schedule(after: .milliseconds(20)) { values.append(2) }
    try await Task.sleep(for: .milliseconds(200))
    precondition(values == [2], "Only the latest scheduled action should run")

    debounce.schedule(after: .milliseconds(20)) { values.append(3) }
    debounce.cancel()
    try await Task.sleep(for: .milliseconds(100))
    precondition(values == [2], "Cancellation must suppress the pending action")

    var temporary: DebouncedTask? = DebouncedTask()
    temporary?.schedule(after: .milliseconds(20)) { values.append(4) }
    temporary = nil
    try await Task.sleep(for: .milliseconds(100))
    precondition(values == [2], "Destroying the owner must cancel pending work")

    let clock = ContinuousClock()
    let start = clock.now
    var timer = PeriodicTimer(interval: .milliseconds(20)).makeAsyncIterator()
    let first = await timer.next()
    precondition(
      first != nil && start.duration(to: first!) >= .milliseconds(20),
      "The first tick must wait for the interval")
    let second = await timer.next()
    precondition(second != nil && start.duration(to: second!) >= .milliseconds(40))

    let waiting = Task {
      var timer = PeriodicTimer(interval: .seconds(60)).makeAsyncIterator()
      let first = await timer.next()
      let second = await timer.next()
      return first == nil && second == nil
    }
    await Task.yield()
    waiting.cancel()
    let cancelled = await waiting.value
    precondition(cancelled, "Cancellation must end the iterator permanently")
    print("Concurrency tests passed")
  }
}
