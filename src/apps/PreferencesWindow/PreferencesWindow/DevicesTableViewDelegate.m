#import "DevicesTableViewDelegate.h"
#import "DevicesTableCellView.h"
#import "DevicesTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "libkrbn.h"

@interface DevicesTableViewDelegate ()

@property(weak) IBOutlet DevicesTableViewController* devicesTableViewController;

@end

@implementation DevicesTableViewDelegate

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
  if ([tableColumn.identifier isEqualToString:@"DevicesCheckboxColumn"]) {
    DevicesTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesCheckboxCellView" owner:self];
    KarabinerKitConnectedDevices* connectedDevices = [KarabinerKitDeviceManager sharedManager].connectedDevices;

    result.checkbox.title = [NSString stringWithFormat:@"%@ (%@)",
                                                       [connectedDevices productAtIndex:row],
                                                       [connectedDevices manufacturerAtIndex:row]];
    result.checkbox.action = @selector(valueChanged:);
    result.checkbox.target = self.devicesTableViewController;

    [result setDeviceIdentifiers:connectedDevices index:row];

    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
    if ([coreConfigurationModel selectedProfileDeviceIgnore:result.deviceVendorId
                                                  productId:result.deviceProductId
                                                 isKeyboard:result.deviceIsKeyboard
                                           isPointingDevice:result.deviceIsPointingDevice]) {
      result.checkbox.state = NSOffState;
    } else {
      result.checkbox.state = NSOnState;
    }

    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"DevicesVendorIdColumn"]) {
    NSTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesVendorIdCellView" owner:self];
    KarabinerKitConnectedDevices* connectedDevices = [KarabinerKitDeviceManager sharedManager].connectedDevices;
    result.textField.stringValue = [NSString stringWithFormat:@"%ld", [connectedDevices vendorIdAtIndex:row]];
    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"DevicesProductIdColumn"]) {
    NSTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesVendorIdCellView" owner:self];
    KarabinerKitConnectedDevices* connectedDevices = [KarabinerKitDeviceManager sharedManager].connectedDevices;
    result.textField.stringValue = [NSString stringWithFormat:@"%ld", [connectedDevices productIdAtIndex:row]];
    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"DevicesIconsColumn"]) {
    DevicesTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesIconsCellView" owner:self];
    KarabinerKitConnectedDevices* connectedDevices = [KarabinerKitDeviceManager sharedManager].connectedDevices;
    result.keyboardImage.hidden = ![connectedDevices isKeyboardAtIndex:row];
    result.mouseImage.hidden = ![connectedDevices isPointingDeviceAtIndex:row];
    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"DevicesExternalKeyboardColumn"]) {
    DevicesTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesExternalKeyboardCellView" owner:self];
    KarabinerKitConnectedDevices* connectedDevices = [KarabinerKitDeviceManager sharedManager].connectedDevices;

    result.checkbox.title = [NSString stringWithFormat:@"%@ (%@) [%ld,%ld]",
                                                       [connectedDevices productAtIndex:row],
                                                       [connectedDevices manufacturerAtIndex:row],
                                                       [connectedDevices vendorIdAtIndex:row],
                                                       [connectedDevices productIdAtIndex:row]];
    result.checkbox.state = NSOffState;

    if ([connectedDevices isBuiltInKeyboardAtIndex:row]) {
      result.checkbox.enabled = NO;
    } else {
      result.checkbox.enabled = YES;
      result.checkbox.action = @selector(valueChanged:);
      result.checkbox.target = self.devicesTableViewController;

      [result setDeviceIdentifiers:connectedDevices index:row];

      KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
      if ([coreConfigurationModel selectedProfileDeviceDisableBuiltInKeyboardIfExists:result.deviceVendorId
                                                                            productId:result.deviceProductId
                                                                           isKeyboard:result.deviceIsKeyboard
                                                                     isPointingDevice:result.deviceIsPointingDevice]) {
        result.checkbox.state = NSOnState;
      } else {
        result.checkbox.state = NSOffState;
      }
    }

    // ----------------------------------------
    result.keyboardImage.hidden = ![connectedDevices isKeyboardAtIndex:row];
    result.mouseImage.hidden = ![connectedDevices isPointingDeviceAtIndex:row];

    // ----------------------------------------
    return result;
  }

  return nil;
}

@end
