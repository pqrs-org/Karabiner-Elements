// -*- Mode: objc -*-

@import Cocoa;
#import "KarabinerKit/KarabinerKit.h"

@interface SimpleModificationsTableViewController : NSObject

+ (void)selectPopUpButtonMenu:(NSPopUpButton*)popUpButton
                   definition:(KarabinerKitCoreConfigurationSimpleModificationsDefinition*)definition;

- (void)setup;
- (void)valueChanged:(id)sender;
- (void)removeItem:(id)sender;

- (void)updateConnectedDevicesMenu;
- (NSInteger)selectedConnectedDeviceIndex;

@end
