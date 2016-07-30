#pragma once

// OS X headers
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <IOKit/hid/IOHIDValue.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <mach/mach_time.h>

// C++ headers
#include <array>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

// C headers
#include <sys/stat.h>
#include <sys/types.h>

// asio headers
#define ASIO_STANDALONE
#include <asio.hpp>

// grabber headers
#include "apple_hid_usage_tables.hpp"
#include "event_grabber.hpp"
#include "grabber_server.hpp"
#include "hid_report.hpp"
#include "human_interface_device.hpp"
#include "session.hpp"
#include "user_client.hpp"
#include "virtual_hid_manager_user_client_method.hpp"
