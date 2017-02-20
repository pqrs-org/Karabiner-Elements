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
  NSArray<KarabinerKitDeviceModel*>* deviceModels = [KarabinerKitDeviceManager sharedManager].deviceModels;
  if (0 <= row && row < (NSInteger)(deviceModels.count)) {
    KarabinerKitDeviceModel* model = deviceModels[row];

    if ([tableColumn.identifier isEqualToString:@"DevicesCheckboxColumn"]) {
      DevicesTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesCheckboxCellView" owner:self];

      NSString* productName = model.deviceDescriptions.product;
      if ([productName length] == 0) {
        productName = @"No product name";
      }
      NSString* manufacturerName = model.deviceDescriptions.manufacturer;
      if ([manufacturerName length] == 0) {
        manufacturerName = @"No manufacturer name";
      }
      result.checkbox.title = [NSString stringWithFormat:@"%@ (%@)", productName, manufacturerName];

      result.checkbox.action = @selector(valueChanged:);
      result.checkbox.target = self.devicesTableViewController;

      result.deviceIdentifiers = model.deviceIdentifiers;

      result.checkbox.state = (model.ignore ? NSOffState : NSOnState);

      KarabinerKitCoreConfigurationModel2* coreConfigurationModel2 = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel2;
      if ([coreConfigurationModel2 selectedProfileDeviceIgnore:model.deviceIdentifiers.vendorId
                                                     productId:model.deviceIdentifiers.productId
                                                    isKeyboard:model.deviceIdentifiers.isKeyboard
                                              isPointingDevice:model.deviceIdentifiers.isPointingDevice]) {
        result.checkbox.state = NSOffState;
      } else {
        result.checkbox.state = NSOnState;
      }

      return result;
    }

    if ([tableColumn.identifier isEqualToString:@"DevicesVendorIdColumn"]) {
      NSTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesVendorIdCellView" owner:self];
      result.textField.stringValue = [NSString stringWithFormat:@"0x%04x", model.deviceIdentifiers.vendorId];
      return result;
    }

    if ([tableColumn.identifier isEqualToString:@"DevicesProductIdColumn"]) {
      NSTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesVendorIdCellView" owner:self];
      result.textField.stringValue = [NSString stringWithFormat:@"0x%04x", model.deviceIdentifiers.productId];
      return result;
    }

    if ([tableColumn.identifier isEqualToString:@"DevicesIconsColumn"]) {
      DevicesTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesIconsCellView" owner:self];
      result.keyboardImage.hidden = !(model.deviceIdentifiers.isKeyboard);
      result.mouseImage.hidden = !(model.deviceIdentifiers.isPointingDevice);
      return result;
    }

    if ([tableColumn.identifier isEqualToString:@"DevicesExternalKeyboardColumn"]) {
      DevicesTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesExternalKeyboardCellView" owner:self];

      // ----------------------------------------
      NSString* productName = model.deviceDescriptions.product;
      if ([productName length] == 0) {
        productName = @"No product name";
      }
      NSString* manufacturerName = model.deviceDescriptions.manufacturer;
      if ([manufacturerName length] == 0) {
        manufacturerName = @"No manufacturer name";
      }
      result.checkbox.title = [NSString stringWithFormat:@"%@ (%@) [0x%04x,0x%04x]",
                                                         productName, manufacturerName,
                                                         model.deviceIdentifiers.vendorId,
                                                         model.deviceIdentifiers.productId];
      result.checkbox.state = NSOffState;

      if (model.isBuiltInKeyboard) {
        result.checkbox.enabled = NO;
      } else {
        result.checkbox.enabled = YES;
        result.checkbox.action = @selector(valueChanged:);
        result.checkbox.target = self.devicesTableViewController;

        result.deviceIdentifiers = model.deviceIdentifiers;

        KarabinerKitCoreConfigurationModel2* coreConfigurationModel2 = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel2;
        if ([coreConfigurationModel2 selectedProfileDeviceDisableBuiltInKeyboardIfExists:model.deviceIdentifiers.vendorId
                                                                               productId:model.deviceIdentifiers.productId
                                                                              isKeyboard:model.deviceIdentifiers.isKeyboard
                                                                        isPointingDevice:model.deviceIdentifiers.isPointingDevice]) {
          result.checkbox.state = NSOnState;
        } else {
          result.checkbox.state = NSOffState;
        }
      }

      // ----------------------------------------
      result.keyboardImage.hidden = !(model.deviceIdentifiers.isKeyboard);
      result.mouseImage.hidden = !(model.deviceIdentifiers.isPointingDevice);

      // ----------------------------------------
      return result;
    }
  }

  return nil;
}

@end
