func mtDeviceRegistryEntryID(of device: MTDevice?) -> UInt64? {
  let service = MTDeviceGetService(device)
  var entryID: UInt64 = 0
  let kr = IORegistryEntryGetRegistryEntryID(service, &entryID)

  guard kr == KERN_SUCCESS else {
    return nil
  }

  return entryID
}

private func callback(
  _ device: MTDevice?,
  _ fingerData: UnsafeMutablePointer<Finger>?,
  _ fingerCount: Int32,
  _ timestamp: Double,
  _ frame: Int32
) -> Int32 {
  let fingers =
    fingerData != nil
    ? Array(UnsafeBufferPointer(start: fingerData, count: Int(fingerCount)))
    : []

  if let device = device {
    if let mtDeviceRegistryEntryID = mtDeviceRegistryEntryID(of: device) {
      Task { @MainActor in
        FingerManager.shared.update(
          mtDeviceRegistryEntryID: mtDeviceRegistryEntryID,
          fingers: fingers,
          timestamp: timestamp,
          frame: frame)
      }
    }
  }

  return 0
}

@MainActor
class MultitouchDeviceManager {
  static let shared = MultitouchDeviceManager()

  private var notificationsTask: Task<Void, Never>?

  private let notificationPort = IONotificationPortCreate(kIOMainPortDefault)

  private var devices: [MTDevice] = []
  private var wakeObserver: NSObjectProtocol?

  init() {
    notificationsTask = Task {
      await withTaskGroup(of: Void.self) { group in
        group.addTask {
          for await _ in NSWorkspace.shared.notificationCenter.notifications(
            named: NSWorkspace.didWakeNotification
          ) {
            Task { @MainActor in
              print("didWakeNotification")

              do {
                // Sleep until devices are settled.
                try await Task.sleep(nanoseconds: NSEC_PER_SEC)

                if UserSettings.shared.relaunchAfterWakeUpFromSleep {
                  try await Task.sleep(
                    nanoseconds: UInt64(UserSettings.shared.relaunchWait) * NSEC_PER_SEC)

                  // MultitouchExtension will be relaunched by launchd.
                  NSApplication.shared.terminate(self)
                }

                MultitouchDeviceManager.shared.setCallback(true)
              } catch {
                print(error.localizedDescription)
              }
            }
          }
        }
      }
    }
  }

  func setCallback(_ register: Bool) {
    print("setCallback \(register)")

    //
    // Unset callback first
    //

    for device in devices {
      MTDeviceStop(device, 0)
      MTUnregisterContactFrameCallback(device, callback)
    }

    devices.removeAll()

    //
    // Set callbacodk
    //

    if register {
      devices = (MTDeviceCreateList().takeUnretainedValue() as? [MTDevice]) ?? []

      for device in devices {
        MTRegisterContactFrameCallback(device, callback)
        MTDeviceStart(device, 0)
      }
    }
  }

  //
  // IONotification
  //

  private func releaseIterator(_ iterator: io_iterator_t) {
    while true {
      let obj = IOIteratorNext(iterator)
      if obj == 0 {
        break
      }

      IOObjectRelease(obj)
    }
  }

  func observeIONotification() {
    print("observeIONotification")

    //
    // Relaunch if device is connected or disconnected
    //

    let match = IOServiceMatching("AppleMultitouchDevice") as NSMutableDictionary

    for notification in [
      kIOMatchedNotification,
      kIOTerminatedNotification,
    ] {
      do {
        var it: io_iterator_t = 0

        let kr = IOServiceAddMatchingNotification(
          notificationPort,
          notification,
          match,
          { _, _ in
            // MultitouchExtension will be relaunched by launchd.
            NSApplication.shared.terminate(nil)
          },
          nil,
          &it)
        if kr != kIOReturnSuccess {
          print("IOServiceAddMatchingNotification error: \(kr)")
          return
        }

        releaseIterator(it)
      }
    }

    let loopSource = IONotificationPortGetRunLoopSource(notificationPort).takeUnretainedValue()
    CFRunLoopAddSource(RunLoop.current.getCFRunLoop(), loopSource, .defaultMode)
  }
}
