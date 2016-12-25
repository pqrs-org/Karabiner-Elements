#import "SimpleModificationsTableViewDelegate.h"
#import "ConfigurationManager.h"
#import "SimpleModificationsMenuManager.h"
#import "SimpleModificationsTableCellView.h"
#import "SimpleModificationsTableViewController.h"

@interface SimpleModificationsTableViewDelegate ()

@property(weak) IBOutlet ConfigurationManager* configurationManager;
@property(weak) IBOutlet SimpleModificationsMenuManager* simpleModificationsMenuManager;
@property(weak) IBOutlet SimpleModificationsTableViewController* simpleModificationsTableViewController;

@end

@implementation SimpleModificationsTableViewDelegate

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
  if ([tableColumn.identifier isEqualToString:@"SimpleModificationsFromColumn"]) {
    SimpleModificationsTableCellView* result = [tableView makeViewWithIdentifier:@"SimpleModificationsFromCellView" owner:self];

    NSArray<NSDictionary*>* simpleModifications = self.configurationManager.coreConfigurationModel.simpleModifications;
    if (0 <= row && row < (NSInteger)(simpleModifications.count)) {
      result.popUpButton.action = @selector(valueChanged:);
      result.popUpButton.target = self.simpleModificationsTableViewController;
      result.popUpButton.menu = [self.simpleModificationsMenuManager.fromMenu copy];

      NSString* fromValue = simpleModifications[row][@"from"];
      [SimpleModificationsTableViewController selectPopUpButtonMenu:result.popUpButton representedObject:fromValue];
    }

    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"SimpleModificationsToColumn"]) {
    SimpleModificationsTableCellView* result = [tableView makeViewWithIdentifier:@"SimpleModificationsToCellView" owner:self];

    NSArray<NSDictionary*>* simpleModifications = self.configurationManager.coreConfigurationModel.simpleModifications;
    if (0 <= row && row < (NSInteger)(simpleModifications.count)) {
      result.popUpButton.action = @selector(valueChanged:);
      result.popUpButton.target = self.simpleModificationsTableViewController;
      result.popUpButton.menu = [self.simpleModificationsMenuManager.toMenu copy];

      NSString* fromValue = simpleModifications[row][@"from"];
      if (!fromValue || [fromValue isEqualToString:@""]) {
        result.popUpButton.enabled = NO;
      } else {
        result.popUpButton.enabled = YES;
      }

      NSString* toValue = simpleModifications[row][@"to"];
      [SimpleModificationsTableViewController selectPopUpButtonMenu:result.popUpButton representedObject:toValue];
    }

    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"SimpleModificationsDeleteColumn"]) {
    SimpleModificationsTableCellView* result = [tableView makeViewWithIdentifier:@"SimpleModificationsDeleteCellView" owner:self];
    result.removeButton.action = @selector(removeItem:);
    result.removeButton.target = self.simpleModificationsTableViewController;

    return result;
  }

  return nil;
}

@end
