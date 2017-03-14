// -*- Mode: objc -*-

@import Foundation;
#import "KarabinerKit/ConfigurationManager.h"
#import "KarabinerKit/ConnectedDevices.h"
#import "KarabinerKit/CoreConfigurationModel.h"
#import "KarabinerKit/DeviceManager.h"
#import "KarabinerKit/JsonUtility.h"
#import "KarabinerKit/NotificationKeys.h"

@interface KarabinerKit : NSObject

+ (void)setup;
+ (void)exitIfAnotherProcessIsRunning:(const char*)pidFileName;

+ (void)observeConsoleUserServerIsDisabledNotification;

+ (void)relaunch;
+ (BOOL)quitKarabinerWithConfirmation;

@end
