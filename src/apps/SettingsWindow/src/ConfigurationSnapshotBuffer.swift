// Holds dispatcher snapshots until all preceding UI requests have completed.
// Revisions also protect against callbacks arriving on the main actor out of order.
struct ConfigurationSnapshotBuffer<Value> {
  private var nextRequestID: UInt64 = 0
  private var pendingRequests: Set<UInt64> = []
  private var lastRevision: UInt64 = 0
  private var pendingSnapshot: (revision: UInt64, value: Value)?

  private(set) var isEditingBlocked = false

  mutating func beginRequest(blocksEditing: Bool = false) -> UInt64 {
    isEditingBlocked = isEditingBlocked || blocksEditing
    nextRequestID += 1
    pendingRequests.insert(nextRequestID)
    return nextRequestID
  }

  mutating func completeRequest(_ id: UInt64) {
    pendingRequests.remove(id)
  }

  mutating func receive(revision: UInt64, value: Value) {
    guard revision > lastRevision, revision > (pendingSnapshot?.revision ?? 0) else { return }
    pendingSnapshot = (revision, value)
  }

  mutating func takeReadySnapshot() -> Value? {
    guard pendingRequests.isEmpty else { return nil }
    // Keep the gate closed until the caller can apply the final snapshot, even
    // if the structural response arrived before an older edit's response.
    isEditingBlocked = false
    guard let snapshot = pendingSnapshot else { return nil }
    lastRevision = snapshot.revision
    pendingSnapshot = nil
    return snapshot.value
  }

  // A delayed stop notification must not reset a newer session's snapshot.
  @discardableResult
  mutating func reset(through revision: UInt64 = 0) -> Bool {
    guard revision == 0 || revision >= max(lastRevision, pendingSnapshot?.revision ?? 0) else {
      return false
    }
    lastRevision = max(revision, max(lastRevision, pendingSnapshot?.revision ?? 0))
    pendingSnapshot = nil
    pendingRequests.removeAll()
    isEditingBlocked = false
    return true
  }
}
