#import "DevicesTableViewDelegate.h"
#import "CoreConfigurationModel.h"
#import "ConfigurationManager.h"
#import "DeviceManager.h"
#import "DevicesTableCellView.h"
#import "DevicesTableViewController.h"

@interface DevicesTableViewDelegate ()

@property(weak) IBOutlet ConfigurationManager* configurationManager;
@property(weak) IBOutlet DeviceManager* deviceManager;
@property(weak) IBOutlet DevicesTableViewController* devicesTableViewController;

@end

@implementation DevicesTableViewDelegate

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
  NSArray<DeviceModel*>* deviceModels = self.deviceManager.deviceModels;
  if (0 <= row && row < (NSInteger)(deviceModels.count)) {
    if ([tableColumn.identifier isEqualToString:@"DevicesCheckboxColumn"]) {
      DevicesTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesCheckboxCellView" owner:self];
      result.checkbox.title = [NSString stringWithFormat:@"%@ (%@)",
                                                         deviceModels[row].deviceDescriptions.product,
                                                         deviceModels[row].deviceDescriptions.manufacturer];
      result.checkbox.action = @selector(valueChanged:);
      result.checkbox.target = self.devicesTableViewController;

      result.deviceIdentifiers = deviceModels[row].deviceIdentifiers;

      result.checkbox.state = (deviceModels[row].ignore ? NSOffState : NSOnState);
      for (DeviceConfiguration* device in self.configurationManager.configurationCoreModel.devices) {
        if ([device.deviceIdentifiers isEqualToDeviceIdentifiers:deviceModels[row].deviceIdentifiers]) {
          result.checkbox.state = (device.ignore ? NSOffState : NSOnState);
        }
      }

      return result;
    }

    if ([tableColumn.identifier isEqualToString:@"DevicesVendorIdColumn"]) {
      NSTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesVendorIdCellView" owner:self];
      result.textField.stringValue = [NSString stringWithFormat:@"0x%04x", deviceModels[row].deviceIdentifiers.vendorId];
      return result;
    }

    if ([tableColumn.identifier isEqualToString:@"DevicesProductIdColumn"]) {
      NSTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesVendorIdCellView" owner:self];
      result.textField.stringValue = [NSString stringWithFormat:@"0x%04x", deviceModels[row].deviceIdentifiers.productId];
      return result;
    }

    if ([tableColumn.identifier isEqualToString:@"DevicesTypeColumn"]) {
      NSTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesTypeCellView" owner:self];
      NSMutableString* type = [NSMutableString new];
      if (deviceModels[row].deviceIdentifiers.isKeyboard) {
        [type appendString:@"keyboard"];
      }
      if (deviceModels[row].deviceIdentifiers.isPointingDevice) {
        if ([type length] > 0) {
          [type appendString:@", "];
        }
        [type appendString:@"pointing device"];
      }
      result.textField.stringValue = type;
      return result;
    }
  }

  return nil;
}

@end
