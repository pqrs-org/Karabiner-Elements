class Debouncer {
  let delay: TimeInterval
  var timer: Timer?

  init(delay: TimeInterval) {
    self.delay = delay
  }

  func debounce(task: @escaping (() -> Void)) {
    timer?.invalidate()
    timer = Timer.scheduledTimer(withTimeInterval: delay, repeats: false) { _ in
      task()
    }
  }
}
