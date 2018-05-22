#import "FnFunctionKeysTableViewDelegate.h"
#import "FnFunctionKeysTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"
#import "SimpleModificationsMenuManager.h"
#import "SimpleModificationsTableCellView.h"
#import "SimpleModificationsTableViewController.h"

@interface FnFunctionKeysTableViewDelegate ()

@property(weak) IBOutlet SimpleModificationsMenuManager* simpleModificationsMenuManager;
@property(weak) IBOutlet FnFunctionKeysTableViewController* fnFunctionKeysTableViewController;

@end

@implementation FnFunctionKeysTableViewDelegate

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
  if ([tableColumn.identifier isEqualToString:@"FnFunctionKeysFromColumn"]) {
    NSTableCellView* result = [tableView makeViewWithIdentifier:@"FnFunctionKeysFromCellView" owner:self];

    result.textField.stringValue = @"";

    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
    NSInteger connectedDeviceIndex = self.fnFunctionKeysTableViewController.selectedConnectedDeviceIndex;
    NSString* jsonString = [coreConfigurationModel selectedProfileFnFunctionKeyFromJsonStringAtIndex:row
                                                                                connectedDeviceIndex:connectedDeviceIndex];
    NSDictionary* json = [KarabinerKitJsonUtility loadCString:jsonString.UTF8String];
    if ([json isKindOfClass:NSDictionary.class]) {
      NSString* s = json[@"key_code"];
      if (s) {
        result.textField.stringValue = s;
      }
    }

    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"FnFunctionKeysToColumn"]) {
    SimpleModificationsTableCellView* result = [tableView makeViewWithIdentifier:@"FnFunctionKeysToCellView" owner:self];

    result.popUpButton.action = @selector(valueChanged:);
    result.popUpButton.target = self.fnFunctionKeysTableViewController;
    if (self.fnFunctionKeysTableViewController.selectedConnectedDeviceIndex == -1) {
      result.popUpButton.menu = [self.simpleModificationsMenuManager.toMenu copy];
    } else {
      result.popUpButton.menu = [self.simpleModificationsMenuManager.toMenuWithInherited copy];
    }

    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
    NSInteger connectedDeviceIndex = self.fnFunctionKeysTableViewController.selectedConnectedDeviceIndex;
    NSString* to = [coreConfigurationModel selectedProfileFnFunctionKeyToJsonStringAtIndex:row
                                                                      connectedDeviceIndex:connectedDeviceIndex];
    [SimpleModificationsTableViewController selectPopUpButtonMenu:result.popUpButton definition:to];

    return result;
  }

  return nil;
}

@end
