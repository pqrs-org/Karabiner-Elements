#import "SimpleModificationsTableViewDelegate.h"
#import "ConfigurationManager.h"

@interface SimpleModificationsTableViewDelegate ()

@property(weak) IBOutlet ConfigurationManager* configurationManager;

@end

@implementation SimpleModificationsTableViewDelegate

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
  if ([tableColumn.identifier isEqualToString:@"SimpleModificationsFromColumn"]) {
    NSTableCellView* result = [tableView makeViewWithIdentifier:@"SimpleModificationsFromCellView" owner:self];
    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"SimpleModificationsToColumn"]) {
    NSTableCellView* result = [tableView makeViewWithIdentifier:@"SimpleModificationsToCellView" owner:self];
    return result;
  }

  return nil;
}

@end
