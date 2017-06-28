#import "ComplexModificationsRulesTableViewDelegate.h"
#import "ComplexModificationsRulesTableCellView.h"
#import "ComplexModificationsRulesTableViewController.h"
#import "KarabinerKit/KarabinerKit.h"

@interface ComplexModificationsRulesTableViewDelegate ()

@property(weak) IBOutlet ComplexModificationsRulesTableViewController* complexModificationsRulesTableViewController;

@end

@implementation ComplexModificationsRulesTableViewDelegate
- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
  if ([tableColumn.identifier isEqualToString:@"ComplexModificationsRulesDescriptionColumn"]) {
    NSTableCellView* result = [tableView makeViewWithIdentifier:@"ComplexModificationsRulesDescriptionCellView" owner:self];

    KarabinerKitCoreConfigurationModel* coreConfigurationModel = [KarabinerKitConfigurationManager sharedManager].coreConfigurationModel;

    result.textField.stringValue = [coreConfigurationModel selectedProfileComplexModificationsRuleDescription:row];

    return result;
  }

  if ([tableColumn.identifier isEqualToString:@"ComplexModificationsRulesRemoveColumn"]) {
    ComplexModificationsRulesTableCellView* result = [tableView makeViewWithIdentifier:@"ComplexModificationsRulesRemoveCellView" owner:self];
    result.removeButton.action = @selector(removeRule:);
    result.removeButton.target = self.complexModificationsRulesTableViewController;

    return result;
  }

  return nil;
}

- (void)tableViewSelectionDidChange:(NSNotification*)notification {
  [self.complexModificationsRulesTableViewController updateUpDownButtons];
}

@end
