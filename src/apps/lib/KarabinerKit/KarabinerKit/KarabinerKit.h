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

+ (BOOL)quitKarabinerWithConfirmation;

@end
