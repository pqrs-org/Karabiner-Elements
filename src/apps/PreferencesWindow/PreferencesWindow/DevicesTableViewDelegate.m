#import "DevicesTableViewDelegate.h"
#import "DevicesTableViewController.h"
#import "ConfigurationCoreModel.h"
#import "ConfigurationManager.h"
#import "DeviceManager.h"
#import "DevicesTableCellView.h"

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
      result.checkbox.title = [NSString stringWithFormat:@"%@ (%@)", deviceModels[row].product, deviceModels[row].manufacturer];
      result.checkbox.action = @selector(valueChanged:);
      result.checkbox.target = self.devicesTableViewController;

      result.vendorId = deviceModels[row].vendorId;
      result.productId = deviceModels[row].productId;

      result.checkbox.state = (deviceModels[row].ignore ? NSOffState : NSOnState);
      for (NSDictionary* device in self.configurationManager.configurationCoreModel.devices) {
        if ([device[@"vendor_id"] unsignedIntValue] == deviceModels[row].vendorId &&
            [device[@"product_id"] unsignedIntValue] == deviceModels[row].productId) {
          result.checkbox.state = ([device[@"ignore"] boolValue] ? NSOffState : NSOnState);
        }
      }

      return result;
    }

    if ([tableColumn.identifier isEqualToString:@"DevicesVendorIdColumn"]) {
      NSTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesVendorIdCellView" owner:self];
      result.textField.stringValue = [NSString stringWithFormat:@"0x%04x", deviceModels[row].vendorId];
      return result;
    }

    if ([tableColumn.identifier isEqualToString:@"DevicesProductIdColumn"]) {
      NSTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesVendorIdCellView" owner:self];
      result.textField.stringValue = [NSString stringWithFormat:@"0x%04x", deviceModels[row].productId];
      return result;
    }
  }

  return nil;
}

@end
