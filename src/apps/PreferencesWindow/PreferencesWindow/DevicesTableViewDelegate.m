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
    libkrbn_device_identifiers deviceIdentifiers = [connectedDevices deviceIdentifiersAtIndex:row];

    result.checkbox.title = [NSString stringWithFormat:@"%@ (%@)",
                                                       [connectedDevices productAtIndex:row],
                                                       [connectedDevices manufacturerAtIndex:row]];
    result.checkbox.action = @selector(valueChanged:);
    result.checkbox.target = self.devicesTableViewController;

    result.deviceIdentifiers = deviceIdentifiers;

    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
    if ([coreConfigurationModel selectedProfileDeviceIgnore:(&deviceIdentifiers)]) {
      result.checkbox.state = NSOffState;
    } else {
      result.checkbox.state = NSOnState;
    }

    result.checkbox.enabled = YES;
    if (libkrbn_device_identifiers_is_apple(&deviceIdentifiers) &&
        deviceIdentifiers.is_pointing_device) {
      result.checkbox.enabled = NO;
    }

    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"DevicesManipulateCapsLockLEDColumn"]) {
    DevicesTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesManipulateCapsLockLEDCellView" owner:self];
    KarabinerKitConnectedDevices* connectedDevices = [KarabinerKitDeviceManager sharedManager].connectedDevices;
    libkrbn_device_identifiers deviceIdentifiers = [connectedDevices deviceIdentifiersAtIndex:row];

    result.checkbox.action = @selector(hasCapsLockLedChanged:);
    result.checkbox.target = self.devicesTableViewController;

    result.deviceIdentifiers = deviceIdentifiers;

    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
    if ([coreConfigurationModel selectedProfileDeviceManipulateCapsLockLed:(&deviceIdentifiers)]) {
      result.checkbox.state = NSOnState;
    } else {
      result.checkbox.state = NSOffState;
    }

    result.checkbox.enabled = YES;
    if (!deviceIdentifiers.is_keyboard) {
      result.checkbox.enabled = NO;
    }

    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"DevicesVendorIdColumn"]) {
    NSTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesVendorIdCellView" owner:self];
    KarabinerKitConnectedDevices* connectedDevices = [KarabinerKitDeviceManager sharedManager].connectedDevices;
    libkrbn_device_identifiers deviceIdentifiers = [connectedDevices deviceIdentifiersAtIndex:row];
    result.textField.stringValue = [NSString stringWithFormat:@"%d", deviceIdentifiers.vendor_id];
    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"DevicesProductIdColumn"]) {
    NSTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesVendorIdCellView" owner:self];
    KarabinerKitConnectedDevices* connectedDevices = [KarabinerKitDeviceManager sharedManager].connectedDevices;
    libkrbn_device_identifiers deviceIdentifiers = [connectedDevices deviceIdentifiersAtIndex:row];
    result.textField.stringValue = [NSString stringWithFormat:@"%d", deviceIdentifiers.product_id];
    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"DevicesIconsColumn"]) {
    DevicesTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesIconsCellView" owner:self];
    KarabinerKitConnectedDevices* connectedDevices = [KarabinerKitDeviceManager sharedManager].connectedDevices;
    libkrbn_device_identifiers deviceIdentifiers = [connectedDevices deviceIdentifiersAtIndex:row];

    result.keyboardImage.hidden = !(deviceIdentifiers.is_keyboard);
    result.mouseImage.hidden = !(deviceIdentifiers.is_pointing_device);
    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"DevicesExternalKeyboardColumn"]) {
    DevicesTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesExternalKeyboardCellView" owner:self];
    KarabinerKitConnectedDevices* connectedDevices = [KarabinerKitDeviceManager sharedManager].connectedDevices;
    libkrbn_device_identifiers deviceIdentifiers = [connectedDevices deviceIdentifiersAtIndex:row];

    result.checkbox.title = [NSString stringWithFormat:@"%@ (%@) [%d,%d]",
                                                       [connectedDevices productAtIndex:row],
                                                       [connectedDevices manufacturerAtIndex:row],
                                                       deviceIdentifiers.vendor_id,
                                                       deviceIdentifiers.product_id];
    result.checkbox.state = NSOffState;

    if ([connectedDevices isBuiltInKeyboardAtIndex:row] ||
        [connectedDevices isBuiltInTrackpadAtIndex:row]) {
      result.checkbox.enabled = NO;
    } else {
      result.checkbox.enabled = YES;
      result.checkbox.action = @selector(valueChanged:);
      result.checkbox.target = self.devicesTableViewController;

      result.deviceIdentifiers = [connectedDevices deviceIdentifiersAtIndex:row];

      KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
      if ([coreConfigurationModel selectedProfileDeviceDisableBuiltInKeyboardIfExists:(&deviceIdentifiers)]) {
        result.checkbox.state = NSOnState;
      } else {
        result.checkbox.state = NSOffState;
      }
    }

    // ----------------------------------------
    result.keyboardImage.hidden = !(deviceIdentifiers.is_keyboard);
    result.mouseImage.hidden = !(deviceIdentifiers.is_pointing_device);

    // ----------------------------------------
    return result;
  }

  return nil;
}

@end
