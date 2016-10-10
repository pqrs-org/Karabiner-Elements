#import "SimpleModificationsTableViewDelegate.h"
#import "ConfigurationManager.h"
#import "SimpleModificationsMenuManager.h"
#import "SimpleModificationsTableCellView.h"

@interface SimpleModificationsTableViewDelegate ()

@property(weak) IBOutlet ConfigurationManager* configurationManager;
@property(weak) IBOutlet SimpleModificationsMenuManager* simpleModificationsMenuManager;

@end

@implementation SimpleModificationsTableViewDelegate

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
  if ([tableColumn.identifier isEqualToString:@"SimpleModificationsFromColumn"]) {
    SimpleModificationsTableCellView* result = [tableView makeViewWithIdentifier:@"SimpleModificationsFromCellView" owner:self];

    NSArray<NSDictionary*>* simpleModifications = self.configurationManager.configurationCoreModel.simpleModifications;
    if (0 <= row && row < (NSInteger)(simpleModifications.count)) {
      result.textField.stringValue = simpleModifications[row][@"from"];
    }

    result.popUpButton.menu = [self.simpleModificationsMenuManager.fromMenu copy];

    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"SimpleModificationsToColumn"]) {
    NSTableCellView* result = [tableView makeViewWithIdentifier:@"SimpleModificationsToCellView" owner:self];

    NSArray<NSDictionary*>* simpleModifications = self.configurationManager.configurationCoreModel.simpleModifications;
    if (0 <= row && row < (NSInteger)(simpleModifications.count)) {
      result.textField.stringValue = simpleModifications[row][@"to"];
    }

    return result;
  }

  return nil;
}

@end
