import ServiceManagement

struct ServiceManagementHelper {
  static func register(services: [SMAppService]) {
    for s in services {
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

      if s.status == .notFound {
        unregister(services: [s])
      }

      do {
        try s.register()
        print("Successfully registered \(s)")
      } catch {
        // Note:
        // When calling `SMAppService.daemon.register`, if user approval has not been granted, an `Operation not permitted` error will be returned.
        // To call `register` for all agents and daemons, the loop continues even if an error occurs.
        // Therefore, only log output will be performed here.
        print("Unable to register \(error)")
      }
    }
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
