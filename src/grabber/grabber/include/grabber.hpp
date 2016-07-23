#pragma once

#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <IOKit/hid/IOHIDValue.h>
#include <mach/mach_time.h>

#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "event_grabber.hpp"
#include "hid_report.hpp"
#include "human_interface_device.hpp"
#include "user_client.hpp"
#include "virtual_hid_manager_user_client_method.hpp"
