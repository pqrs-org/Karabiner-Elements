import Foundation
import ServiceManagement

enum Subcommand: String {
  case registerCoreAgents = "register-core-agents"
  case unregisterCoreAgents = "unregister-core-agents"
  case coreAgentsEnabled = "core-agents-enabled"

  case registerMenuAgent = "register-menu-agent"
  case unregisterMenuAgent = "unregister-menu-agent"

  case registerMultitouchExtensionAgent = "register-multitouch-extension-agent"
  case unregisterMultitouchExtensionAgent = "unregister-multitouch-extension-agent"

  case registerNotificationWindowAgent = "register-notification-window-agent"
  case unregisterNotificationWindowAgent = "unregister-notification-window-agent"

  case status
  case running
}

RunLoop.main.perform {
  let coreAgentServiceNames = [
    "org.pqrs.service.agent.karabiner_console_user_server",
    "org.pqrs.service.agent.karabiner_session_monitor",
    "org.pqrs.service.agent.Karabiner-Core-Service",
  ]

  let coreAgents = coreAgentServiceNames.map {
    SMAppService.agent(plistName: "\($0).plist")
  }

  let menuAgentService = SMAppService.agent(
    plistName: "org.pqrs.service.agent.Karabiner-Menu.plist")

  let multitouchExtensionAgentService = SMAppService.agent(
    plistName: "org.pqrs.service.agent.Karabiner-MultitouchExtension.plist")

  let notificationWindowAgentService = SMAppService.agent(
    plistName: "org.pqrs.service.agent.Karabiner-NotificationWindow.plist")

  var allServices: [SMAppService] = []
  for s in coreAgents {
    allServices.append(s)
  }
  allServices.append(menuAgentService)
  allServices.append(multitouchExtensionAgentService)
  allServices.append(notificationWindowAgentService)

  if CommandLine.arguments.count > 1 {
    let subcommand = CommandLine.arguments[1]

    switch Subcommand(rawValue: subcommand) {
    case .registerCoreAgents:
      ServiceManagementHelper.register(services: coreAgents)
      exit(0)

    case .unregisterCoreAgents:
      ServiceManagementHelper.unregister(services: coreAgents)
      exit(0)

    case .coreAgentsEnabled:
      if ServiceManagementHelper.enabled(services: coreAgents) {
        print("enabled")
        exit(0)
      } else {
        print("There are services that are not enabled")
        exit(1)
      }

    case .registerMenuAgent:
      ServiceManagementHelper.register(services: [menuAgentService])
      exit(0)

    case .unregisterMenuAgent:
      ServiceManagementHelper.unregister(services: [menuAgentService])
      exit(0)

    case .registerMultitouchExtensionAgent:
      ServiceManagementHelper.register(services: [multitouchExtensionAgentService])
      exit(0)

    case .unregisterMultitouchExtensionAgent:
      ServiceManagementHelper.unregister(services: [multitouchExtensionAgentService])
      exit(0)

    case .registerNotificationWindowAgent:
      ServiceManagementHelper.register(services: [notificationWindowAgentService])
      exit(0)

    case .unregisterNotificationWindowAgent:
      ServiceManagementHelper.unregister(services: [notificationWindowAgentService])
      exit(0)

    case .status:
      ServiceManagementHelper.printStatuses(services: allServices)
      exit(0)

    case .running:
      var exitCode: Int32 = 0
      for n in coreAgentServiceNames {
        // A non-resident agent that runs only once
        if n == "org.pqrs.service.agent.Karabiner-Core-Service" {
          continue
        }

        n.withCString {
          if !libkrbn_services_agent_running($0) {
            print("\(n) is not running")
            exitCode = 1
          }
        }
      }
      exit(exitCode)

    default:
      print("Unknown subcommand \(subcommand)")
      exit(1)
    }
  }

  print("Usage:")
  print("    'Karabiner-Elements Non-Privileged Agents' subcommand")
  print("")
  print("Subcommands:")
  print("    \(Subcommand.registerCoreAgents.rawValue)")
  print("    \(Subcommand.unregisterCoreAgents.rawValue)")
  print("    \(Subcommand.coreAgentsEnabled.rawValue)")

  print("    \(Subcommand.registerMenuAgent.rawValue)")
  print("    \(Subcommand.unregisterMenuAgent.rawValue)")

  print("    \(Subcommand.registerMultitouchExtensionAgent.rawValue)")
  print("    \(Subcommand.unregisterMultitouchExtensionAgent.rawValue)")

  print("    \(Subcommand.registerNotificationWindowAgent.rawValue)")
  print("    \(Subcommand.unregisterNotificationWindowAgent.rawValue)")

  print("    \(Subcommand.status.rawValue)")
  print("    \(Subcommand.running.rawValue)")

  exit(0)
}

RunLoop.main.run()
