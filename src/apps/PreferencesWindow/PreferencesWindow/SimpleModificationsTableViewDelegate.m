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
  
  if ([tableColumn.identifier isEqualToString:@"SimpleModificationsVendorIdColumn"]) {
    SimpleModificationsTableCellView* result = [tableView makeViewWithIdentifier:@"SimpleModificationsVendorIdCellView" owner:self];
    
    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
    
    result.popUpButton.action = @selector(valueChanged:);
    result.popUpButton.target = self.simpleModificationsTableViewController;
    result.popUpButton.menu = [self.simpleModificationsMenuManager.vendorIdMenu copy];
    
    NSUInteger vid = [coreConfigurationModel selectedProfileSimpleModificationVendorIdAtIndex:row];
    NSUInteger pid = [coreConfigurationModel selectedProfileSimpleModificationProductIdAtIndex:row];
    NSString *repObj = [NSString stringWithFormat:@"0x%04lx, 0x%04lx", vid, pid];
    
    NSLog(@"Vid/Pid: %@", repObj);
        
    [SimpleModificationsTableViewController selectPopUpButtonMenu:result.popUpButton representedObject:repObj];
    
    return result;
  }
  
  if ([tableColumn.identifier isEqualToString:@"SimpleModificationsProductIdColumn"]) {
    SimpleModificationsTableCellView* result = [tableView makeViewWithIdentifier:@"SimpleModificationsProductIdCellView" owner:self];
    
    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;
    
    result.popUpButton.action = @selector(valueChanged:);
    result.popUpButton.target = self.simpleModificationsTableViewController;
    result.popUpButton.menu = [self.simpleModificationsMenuManager.productIdMenu copy];
    
    NSUInteger id_ = [coreConfigurationModel selectedProfileSimpleModificationProductIdAtIndex:row];
    NSString *repObj = [NSString stringWithFormat:@"%lu", id_];

    [SimpleModificationsTableViewController selectPopUpButtonMenu:result.popUpButton representedObject:repObj];
    
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
