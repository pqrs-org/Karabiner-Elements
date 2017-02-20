#import "SimpleModificationsTableViewDelegate.h"
#import "KarabinerKit/KarabinerKit.h"
#import "SimpleModificationsMenuManager.h"
#import "SimpleModificationsTableCellView.h"
#import "SimpleModificationsTableViewController.h"

@interface SimpleModificationsTableViewDelegate ()

@property(weak) IBOutlet SimpleModificationsMenuManager* simpleModificationsMenuManager;
@property(weak) IBOutlet SimpleModificationsTableViewController* simpleModificationsTableViewController;

@end

@implementation SimpleModificationsTableViewDelegate

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
  if ([tableColumn.identifier isEqualToString:@"SimpleModificationsFromColumn"]) {
    SimpleModificationsTableCellView* result = [tableView makeViewWithIdentifier:@"SimpleModificationsFromCellView" owner:self];

    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;

    result.popUpButton.action = @selector(valueChanged:);
    result.popUpButton.target = self.simpleModificationsTableViewController;
    result.popUpButton.menu = [self.simpleModificationsMenuManager.fromMenu copy];

    NSString* fromValue = [coreConfigurationModel selectedProfileSimpleModificationFirstAtIndex:row];
    [SimpleModificationsTableViewController selectPopUpButtonMenu:result.popUpButton representedObject:fromValue];

    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"SimpleModificationsToColumn"]) {
    SimpleModificationsTableCellView* result = [tableView makeViewWithIdentifier:@"SimpleModificationsToCellView" owner:self];

    result.popUpButton.action = @selector(valueChanged:);
    result.popUpButton.target = self.simpleModificationsTableViewController;
    result.popUpButton.menu = [self.simpleModificationsMenuManager.toMenu copy];

    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
    NSString* fromValue = [coreConfigurationModel selectedProfileSimpleModificationFirstAtIndex:row];
    if ([fromValue length] > 0) {
      result.popUpButton.enabled = YES;
    } else {
      result.popUpButton.enabled = NO;
    }

    NSString* toValue = [coreConfigurationModel selectedProfileSimpleModificationSecondAtIndex:row];
    [SimpleModificationsTableViewController selectPopUpButtonMenu:result.popUpButton representedObject:toValue];

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
