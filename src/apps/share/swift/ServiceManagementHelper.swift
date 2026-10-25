import Foundation
import ServiceManagement

struct ServiceManagementHelper {
  static func launchdLabel(
    plistName: String,
    directoryName: String
  ) -> String? {
    let url = Bundle.main.bundleURL
      .appendingPathComponent("Contents/Library", isDirectory: true)
      .appendingPathComponent(directoryName, isDirectory: true)
      .appendingPathComponent(plistName, isDirectory: false)

    do {
      let data = try Data(contentsOf: url)
      guard
        let dictionary = try PropertyListSerialization.propertyList(
          from: data,
          options: [],
          format: nil
        ) as? [String: Any],
        let label = dictionary["Label"] as? String
      else {
        print("Label is missing in \(url.path)")
        return nil
      }

      return label
    } catch {
      print("Unable to read \(url.path): \(error)")
      return nil
    }
  }

  static func register(services: [SMAppService]) -> Bool {
    // backgroundtaskmanagementd may not discover a newly installed helper app
    // immediately. Retry services affected by this transient .notFound state
    // up to 10 times at one-second intervals.
    let maxRetryCount = 10
    var retryCount = 0
    var pendingServices = services
    var firstAttempt = true

    while true {
      var retryCandidates: [SMAppService] = []

      for s in pendingServices {
        let notFound = s.status == .notFound

        // Regarding daemons, performing the following steps causes inconsistencies in the user approval database,
        // so the process will not start again until it is unregistered and then registered again.
        //
        // 1. Register a daemon.
        // 2. Approve the daemon.
        // 3. The database is reset using `sfltool resetbtm`.
        // 4. Restart macOS.
        //
        // When this happens, the service status becomes .notFound.
        // So, if the service status is .notFound, we call unregister before register to avoid this issue.
        //
        // Another case where it becomes .notFound is when it has never actually been registered before.
        // Even in this case, calling unregister will not have any negative impact.

        if firstAttempt && notFound {
          unregister(services: [s])
        }

        do {
          try s.register()
          print("Successfully registered \(s)")
        } catch {
          // Note:
          // When calling `SMAppService.daemon.register`, if user approval has not been granted, an `Operation not permitted` error will be returned.
          // To call `register` for all agents and daemons, the loop continues even if an error occurs.
          // The final service status determines the return value.
          print("Unable to register \(error)")
        }

        if !registered(s) && (notFound || s.status == .notFound) {
          retryCandidates.append(s)
        }
      }

      if retryCandidates.isEmpty || retryCount >= maxRetryCount {
        break
      }

      Thread.sleep(forTimeInterval: 1)
      retryCount += 1
      pendingServices = retryCandidates
      firstAttempt = false
    }

    return services.allSatisfy(registered)
  }

  private static func registered(_ service: SMAppService) -> Bool {
    service.status == .enabled || service.status == .requiresApproval
  }

  static func unregister(services: [SMAppService]) {
    for s in services {
      do {
        try s.unregister()
        print("Successfully unregistered \(s)")
      } catch {
        print("Unable to unregister \(error)")
      }
    }
  }

  static func enabled(services: [SMAppService]) -> Bool {
    return services.allSatisfy { $0.status == .enabled }
  }

  static func printStatuses(services: [SMAppService]) {
    for s in services {
      switch s.status {
      case .notRegistered:
        // A service that was once registered but then unregistered becomes notRegistered.
        print("\(s) notRegistered")
      case .enabled:
        print("\(s) enabled")
      case .requiresApproval:
        print("\(s) requiresApproval")
      case .notFound:
        // A service that has never been registered becomes notFound. Resetting the database with `sfltool resetbtm` also results in notFound.
        print("\(s) notFound")
      @unknown default:
        print("\(s) unknown \(s.status)")
      }
    }
  }
}
