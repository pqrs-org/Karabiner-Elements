// -*- Mode: objc -*-

#import "KarabinerKit/ConfigurationManager.h"
#import "KarabinerKit/CoreConfigurationModel.h"
#import "KarabinerKit/DeviceManager.h"
#import "KarabinerKit/DeviceModel.h"
#import "KarabinerKit/JsonUtility.h"
#import "KarabinerKit/NotificationKeys.h"
@import Foundation;

@interface KarabinerKit : NSObject

+ (void)setup;
+ (void)exitIfAnotherProcessIsRunning:(const char*)pidFileName;

+ (void)observeConsoleUserServerIsDisabledNotification;

+ (void)relaunch;
+ (BOOL)quitKarabinerWithConfirmation;

@end
