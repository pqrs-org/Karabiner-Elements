#import "FnFunctionKeysTableViewDelegate.h"
#import "ConfigurationManager.h"
#import "FnFunctionKeysTableViewController.h"
#import "SimpleModificationsMenuManager.h"
#import "SimpleModificationsTableCellView.h"
#import "SimpleModificationsTableViewController.h"

@interface FnFunctionKeysTableViewDelegate ()

@property(weak) IBOutlet ConfigurationManager* configurationManager;
@property(weak) IBOutlet SimpleModificationsMenuManager* simpleModificationsMenuManager;
@property(weak) IBOutlet FnFunctionKeysTableViewController* fnFunctionKeysTableViewController;

@end

@implementation FnFunctionKeysTableViewDelegate

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
  if ([tableColumn.identifier isEqualToString:@"FnFunctionKeysFromColumn"]) {
    NSTableCellView* result = [tableView makeViewWithIdentifier:@"FnFunctionKeysFromCellView" owner:self];

    NSArray<NSDictionary*>* fnFunctionKeys = self.configurationManager.configurationCoreModel.fnFunctionKeys;
    if (0 <= row && row < (NSInteger)(fnFunctionKeys.count)) {
      result.textField.stringValue = fnFunctionKeys[row][@"from"];
    }

    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"FnFunctionKeysToColumn"]) {
    SimpleModificationsTableCellView* result = [tableView makeViewWithIdentifier:@"FnFunctionKeysToCellView" owner:self];

    NSArray<NSDictionary*>* fnFunctionKeys = self.configurationManager.configurationCoreModel.fnFunctionKeys;
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
