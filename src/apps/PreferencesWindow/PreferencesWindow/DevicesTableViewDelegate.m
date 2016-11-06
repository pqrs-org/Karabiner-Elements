#import "DevicesTableViewDelegate.h"
#import "ConfigurationManager.h"
#import "CoreConfigurationModel.h"
#import "DeviceManager.h"
#import "DevicesTableCellView.h"
#import "DevicesTableViewController.h"
#import "libkrbn.h"

@interface DevicesTableViewDelegate ()

@property(weak) IBOutlet ConfigurationManager* configurationManager;
@property(weak) IBOutlet DeviceManager* deviceManager;
@property(weak) IBOutlet DevicesTableViewController* devicesTableViewController;

- (void)createKeyboardTypeMenu:(NSPopUpButton*)popUpButton keyboardType:(uint32_t)keyboardType;

@end

@implementation DevicesTableViewDelegate

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
  NSArray<DeviceModel*>* deviceModels = self.deviceManager.deviceModels;
  if (0 <= row && row < (NSInteger)(deviceModels.count)) {
    DeviceModel* model = deviceModels[row];

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
      for (DeviceConfiguration* device in self.configurationManager.configurationCoreModel.devices) {
        if ([device.deviceIdentifiers isEqualToDeviceIdentifiers:model.deviceIdentifiers]) {
          result.checkbox.state = (device.ignore ? NSOffState : NSOnState);
        }
      }

      return result;
    }

    if ([tableColumn.identifier isEqualToString:@"DevicesKeyboardTypeColumn"]) {
      DevicesTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesKeyboardTypeCellView" owner:self];
      result.popUpButton.action = @selector(valueChanged:);
      result.popUpButton.target = self.devicesTableViewController;

      uint32_t keyboardType = 0;
      for (DeviceConfiguration* device in self.configurationManager.configurationCoreModel.devices) {
        if ([device.deviceIdentifiers isEqualToDeviceIdentifiers:model.deviceIdentifiers]) {
          keyboardType = device.keyboardType;
        }
      }

      [self createKeyboardTypeMenu:result.popUpButton keyboardType:keyboardType];
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

        for (DeviceConfiguration* device in self.configurationManager.configurationCoreModel.devices) {
          if ([device.deviceIdentifiers isEqualToDeviceIdentifiers:model.deviceIdentifiers]) {
            result.checkbox.state = (device.disableBuiltInKeyboardIfExists ? NSOnState : NSOffState);
          }
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

- (void)createKeyboardTypeMenu:(NSPopUpButton*)popUpButton keyboardType:(uint32_t)keyboardType {
  popUpButton.menu = [NSMenu new];
  {
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"Default"
                                                  action:NULL
                                           keyEquivalent:@""];
    item.representedObject = [NSNumber numberWithUnsignedInt:0];
    [popUpButton.menu addItem:item];
  }
  {
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"ANSI"
                                                  action:NULL
                                           keyEquivalent:@""];
    item.representedObject = [NSNumber numberWithUnsignedInt:libkrbn_get_keyboard_type_ansi()];
    [popUpButton.menu addItem:item];
  }
  {
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"ISO"
                                                  action:NULL
                                           keyEquivalent:@""];
    item.representedObject = [NSNumber numberWithUnsignedInt:libkrbn_get_keyboard_type_iso()];
    [popUpButton.menu addItem:item];
  }
  {
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"JIS"
                                                  action:NULL
                                           keyEquivalent:@""];
    item.representedObject = [NSNumber numberWithUnsignedInt:libkrbn_get_keyboard_type_jis()];
    [popUpButton.menu addItem:item];
  }

  // ----------------------------------------
  // Select item

  for (NSMenuItem* item in popUpButton.itemArray) {
    if ([item.representedObject unsignedIntValue] == keyboardType) {
      [popUpButton selectItem:item];
      return;
    }
  }

  {
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"type:%d", keyboardType]
                                                  action:NULL
                                           keyEquivalent:@""];
    item.representedObject = [NSNumber numberWithUnsignedInt:keyboardType];
    [popUpButton.menu addItem:item];
    [popUpButton selectItem:item];
  }
}

@end
