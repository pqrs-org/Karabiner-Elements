import Foundation
import ServiceManagement

enum Subcommand: String {
  case registerCoreAgents = "register-core-agents"
  case unregisterCoreAgents = "unregister-core-agents"

  case registerMenuAgent = "register-menu-agent"
  case unregisterMenuAgent = "unregister-menu-agent"

  case registerMultitouchExtensionAgent = "register-multitouch-extension-agent"
  case unregisterMultitouchExtensionAgent = "unregister-multitouch-extension-agent"

  case registerNotificationWindowAgent = "register-notification-window-agent"
  case unregisterNotificationWindowAgent = "unregister-notification-window-agent"

  case status = "status"
}

RunLoop.main.perform {
  let coreAgents: [SMAppService] = [
    SMAppService.agent(
      plistName: "org.pqrs.service.agent.karabiner_console_user_server.plist"),
    SMAppService.agent(
      plistName: "org.pqrs.service.agent.karabiner_grabber.plist"),
    SMAppService.agent(
      plistName: "org.pqrs.service.agent.karabiner_session_monitor.plist"),
  ]

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

    default:
      print("Unknown subcommand \(subcommand)")
      exit(1)
    }
  }

  print("Usage:")
  print("    Karabiner-Elements-Non-Privileged-Agents subcommand")
  print("")
  print("Subcommands:")
  print("    \(Subcommand.registerCoreAgents.rawValue)")
  print("    \(Subcommand.unregisterCoreAgents.rawValue)")

  print("    \(Subcommand.registerMenuAgent.rawValue)")
  print("    \(Subcommand.unregisterMenuAgent.rawValue)")

  print("    \(Subcommand.registerMultitouchExtensionAgent.rawValue)")
  print("    \(Subcommand.unregisterMultitouchExtensionAgent.rawValue)")

  print("    \(Subcommand.registerNotificationWindowAgent.rawValue)")
  print("    \(Subcommand.unregisterNotificationWindowAgent.rawValue)")

  print("    \(Subcommand.status.rawValue)")

  exit(0)
}

RunLoop.main.run()
