#import "FnFunctionKeysTableViewDelegate.h"
#import "KarabinerKit/KarabinerKit.h"
#import "FnFunctionKeysTableViewController.h"
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

    KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];
    NSArray<NSDictionary*>* fnFunctionKeys = configurationManager.coreConfigurationModel.fnFunctionKeys;
    if (0 <= row && row < (NSInteger)(fnFunctionKeys.count)) {
      result.textField.stringValue = fnFunctionKeys[row][@"from"];
    }

    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"FnFunctionKeysToColumn"]) {
    SimpleModificationsTableCellView* result = [tableView makeViewWithIdentifier:@"FnFunctionKeysToCellView" owner:self];

    KarabinerKitConfigurationManager* configurationManager = [KarabinerKitConfigurationManager sharedManager];
    NSArray<NSDictionary*>* fnFunctionKeys = configurationManager.coreConfigurationModel.fnFunctionKeys;
    if (0 <= row && row < (NSInteger)(fnFunctionKeys.count)) {
      result.popUpButton.action = @selector(valueChanged:);
      result.popUpButton.target = self.fnFunctionKeysTableViewController;
      result.popUpButton.menu = [self.simpleModificationsMenuManager.toMenu copy];

      NSString* toValue = fnFunctionKeys[row][@"to"];
      [SimpleModificationsTableViewController selectPopUpButtonMenu:result.popUpButton representedObject:toValue];
    }

    return result;
  }

  return nil;
}

@end
