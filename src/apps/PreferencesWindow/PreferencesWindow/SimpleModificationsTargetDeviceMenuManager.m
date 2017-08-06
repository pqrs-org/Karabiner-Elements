#import "SimpleModificationsTargetDeviceMenuManager.h"
#import "KarabinerKit/KarabinerKit.h"

@implementation SimpleModificationsTargetDeviceMenuManager

+ (NSMenu*)createMenu {
  NSMenu* menu = [NSMenu new];
  menu.autoenablesItems = NO;

  NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"For all devices"
                                                action:NULL
                                         keyEquivalent:@""];
  [menu addItem:item];

  KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
  KarabinerKitConnectedDevices* devices = [KarabinerKitDeviceManager sharedManager].connectedDevices;
  NSUInteger count = devices.devicesCount;
  for (NSUInteger i = 0; i < count; ++i) {
    item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"%@ (%@)",
                                                                        [devices productAtIndex:i],
                                                                        [devices manufacturerAtIndex:i]]
                                      action:NULL
                               keyEquivalent:@""];

    if ([coreConfigurationModel selectedProfileDeviceIgnore:[devices vendorIdAtIndex:i]
                                                  productId:[devices productIdAtIndex:i]
                                                 isKeyboard:[devices isKeyboardAtIndex:i]
                                           isPointingDevice:[devices isPointingDeviceAtIndex:i]]) {
      item.enabled = NO;
    }

    [menu addItem:item];
  }

  return menu;
}

@end
