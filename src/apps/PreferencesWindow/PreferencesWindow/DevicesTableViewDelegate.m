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

- (void) createKeyboardTypeMenu: (NSPopUpButton*) popUpButton keyboardType:(uint32_t) keyboardType;

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

    if ([tableColumn.identifier isEqualToString:@"DevicesKeyboardTypeColumn"]) {
      DevicesTableCellView* result = [tableView makeViewWithIdentifier:@"DevicesKeyboardTypeCellView" owner:self];
      result.popUpButton.action = @selector(valueChanged:);
      result.popUpButton.target = self.devicesTableViewController;
      [self createKeyboardTypeMenu: result.popUpButton keyboardType:deviceModels[row].keyboardType];
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

- (void) createKeyboardTypeMenu:  (NSPopUpButton*) popUpButton keyboardType:(uint32_t) keyboardType {
  popUpButton.menu = [NSMenu new];
  NSMenuItem* item0 = [[NSMenuItem alloc] initWithTitle:@"default"
                                                  action:NULL
                                           keyEquivalent:@""];
  item0.representedObject = [NSNumber numberWithInt:0];
  [popUpButton.menu addItem:item0];
  NSMenuItem* item40 = [[NSMenuItem alloc] initWithTitle:@"ansi"
                                                action:NULL
                                         keyEquivalent:@""];
  item40.representedObject = [NSNumber numberWithInt:40];
  [popUpButton.menu addItem:item40];
  NSMenuItem* item41 = [[NSMenuItem alloc] initWithTitle:@"iso"
                                                  action:NULL
                                           keyEquivalent:@""];
  item41.representedObject = [NSNumber numberWithInt:41];
  [popUpButton.menu addItem:item41];
  NSMenuItem* item42 = [[NSMenuItem alloc] initWithTitle:@"jis"
                                                  action:NULL
                                           keyEquivalent:@""];
  item42.representedObject = [NSNumber numberWithInt:42];
  [popUpButton.menu addItem:item42];

  if(keyboardType == 0){
    [popUpButton selectItem:item0];
  }else if(keyboardType == 40){
    [popUpButton selectItem:item40];
  }else if(keyboardType == 41){
    [popUpButton selectItem:item41];
  }else if(keyboardType == 42){
    [popUpButton selectItem:item42];
  }else{
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"type:%d", keyboardType]
                                                    action:NULL
                                             keyEquivalent:@""];
    item.representedObject = [NSNumber numberWithInt:keyboardType];
    [popUpButton.menu addItem:item];
    [popUpButton selectItem:item];
  }
}

@end
