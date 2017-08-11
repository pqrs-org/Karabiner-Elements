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

    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
    NSInteger connectedDeviceIndex = self.fnFunctionKeysTableViewController.selectedConnectedDeviceIndex;
    result.textField.stringValue = [coreConfigurationModel selectedProfileFnFunctionKeyFirstAtIndex:row
                                                                               connectedDeviceIndex:connectedDeviceIndex];

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
    NSString* toValue = [coreConfigurationModel selectedProfileFnFunctionKeySecondAtIndex:row
                                                                     connectedDeviceIndex:connectedDeviceIndex];
    [SimpleModificationsTableViewController selectPopUpButtonMenu:result.popUpButton representedObject:toValue];

    return result;
  }

  return nil;
}

@end
